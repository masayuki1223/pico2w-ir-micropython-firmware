import network
import socket
import time
import ir
from config import SSID, PASS
from pulses import HEAT_25, COOL_25, FAN_LOW, POWER_OFF
from machine import WDT, Pin

# ===== 設定 =====
STATIC_IP = ("192.168.0.203", "255.255.255.0", "192.168.0.1", "192.168.0.1")
PORT = 5000

wdt = WDT(timeout=8000)
wlan = network.WLAN(network.STA_IF)

led = Pin("LED", Pin.OUT)

# 処理中フラグ & 保留コマンド
processing = False
pending_cmd = None

# ===== 初回接続 =====
def ensure_wifi():
    wlan.active(False)
    time.sleep_ms(500)
    wlan.active(True)
    wlan.config(pm=0xa11140)
    time.sleep_ms(200)
    wlan.ifconfig(STATIC_IP)
    wlan.connect(SSID, PASS)

    for _ in range(25):
        if wlan.isconnected():
            print("Wi-Fi connected:", wlan.ifconfig()[0])
            return True
        time.sleep_ms(200)
        wdt.feed()

    print("Wi-Fi connect failed")
    return False

ensure_wifi()
ir.init()

# ===== TCP サーバ =====
def start_server():
    addr = socket.getaddrinfo("0.0.0.0", PORT)[0][-1]
    s = socket.socket()
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(addr)
    s.listen(1)
    s.settimeout(1)  # 受信ループを回し続けるためのタイムアウト
    print("Listening on", addr)
    return s

server = start_server()

# ===== コマンド分類（軽量） =====
def classify_command(cmd):
    if cmd.startswith("IR:"):
        return cmd[3:]
    return None

# ===== IR 実行ルーチン（重い処理はこちら） =====
def process_pending():
    global processing, pending_cmd

    if not processing or pending_cmd is None:
        return

    mode = classify_command(pending_cmd)

    if not mode:
        # 想定外だが一応クリア
        processing = False
        pending_cmd = None
        return

    print("IR start:", mode)
    led.on()

    if mode == "HEAT25":
        ir.send(HEAT_25)
    elif mode == "COOL25":
        ir.send(COOL_25)
    elif mode == "FANLOW":
        ir.send(FAN_LOW)
    elif mode == "OFF":
        ir.send(POWER_OFF)
    else:
        # 不明コマンド
        processing = False
        pending_cmd = None
        return

    print("IR done:", mode)

    # 処理完了
    processing = False
    pending_cmd = None

# ===== メインループ =====
last_ping = time.ticks_ms()

while True:
    wdt.feed()
    led.off()

    # まず重い処理（保留コマンド）を進める
    process_pending()

    # 受信ループ（軽量）
    try:
        try:
            conn, addr = server.accept()
        except OSError:
            # タイムアウト → Wi-Fi 死んでたら復旧
            if not wlan.isconnected():
                ensure_wifi()
            continue

        conn.settimeout(1)

        try:
            data = conn.recv(128)
        except OSError:
            conn.close()
            continue

        if not data:
            conn.close()
            continue

        cmd_str = data.decode().strip()
        mode = classify_command(cmd_str)

        # IR コマンド
        if mode:
            if processing:
                # すでに1件処理中 → BUSY
                conn.send(b"BUSY")
            else:
                # 受け付けて OK を返すだけ
                pending_cmd = cmd_str
                processing = True
                conn.send(b"OK")
            conn.close()
            continue

        # 不明コマンド
        conn.send(b"NG")
        conn.close()

    except Exception as e:
        print("server error:", e)
        try:
            server.close()
        except:
            pass
        time.sleep_ms(500)
        server = start_server()
