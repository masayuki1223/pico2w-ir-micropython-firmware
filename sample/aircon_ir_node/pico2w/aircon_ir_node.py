import network
import socket
import time
import ir
from pulses import HEAT_25, COOL_25, FAN_LOW, POWER_OFF
from machine import WDT
from machine import Pin

# ===== 設定 =====
SSID = "xxxxxxxxxxxxxxxx"
PASS = "xxxxxxxxxxxxxxxx"
STATIC_IP = ("192.168.0.203", "255.255.255.0", "192.168.0.1", "192.168.0.1")
PORT = 5000

wdt = WDT(timeout=8000)
wlan = network.WLAN(network.STA_IF)

led = Pin("LED", Pin.OUT)
led.off()

# 処理中フラグ & 保留コマンド
processing = False
pending_cmd = None
pending_ip = None

# ===== Wi-Fi 安全停止 =====
def wifi_safe_off():
    if wlan.active():
        wlan.active(False)
        time.sleep_ms(1000)

# ===== Wi-Fi 再起動（接続完了まで待つ） =====
def wifi_safe_on():
    wlan.active(True)
    time.sleep_ms(200)
    wlan.ifconfig(STATIC_IP)
    wlan.connect(SSID, PASS)

    for _ in range(25):  # 最大5秒
        if wlan.isconnected():
            return True
        time.sleep_ms(200)
        wdt.feed()
    return False

# ===== 初回接続 =====
def ensure_wifi():
    wlan.active(False)
    time.sleep_ms(500)
    wlan.active(True)
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
    s.listen(5)
    s.settimeout(1)  # 受信ループを回し続けるためのタイムアウト
    print("Listening on", addr)
    return s

server = start_server()

# ===== Zero2W に通知（/success） =====
def notify_zero2w(ip):
    try:
        import usocket as socket
        port = 5000
        path = "/success"

        addr = socket.getaddrinfo(ip, port)[0][-1]
        s = socket.socket()
        s.connect(addr)

        body = "SUCCESS"
        req = (
            "POST {} HTTP/1.1\r\n"
            "Host: {}\r\n"
            "Content-Type: text/plain\r\n"
            "Content-Length: {}\r\n"
            "\r\n"
            "{}"
        ).format(path, ip, len(body), body)

        s.send(req.encode())
        s.close()
    except Exception as e:
        print("notify error:", e)

# ===== コマンド分類（軽量） =====
def classify_command(cmd):
    cmd = cmd.strip()
    if cmd == "PING":
        return "PING", None
    if cmd.startswith("IR:"):
        return "IR", cmd[3:]
    return "NG", None

# ===== IR 実行ルーチン（重い処理はこちら） =====
def process_pending():
    global processing, pending_cmd, pending_ip

    if not processing or pending_cmd is None or pending_ip is None:
        return

    kind, mode = classify_command(pending_cmd)

    if kind != "IR":
        # 想定外だが一応クリア
        processing = False
        pending_cmd = None
        pending_ip = None
        return

    print("IR start:", mode, "to", pending_ip)
    led.on()

    # Wi-Fi OFF → IR → Wi-Fi ON
    wifi_safe_off()

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
        wifi_safe_on()
        processing = False
        pending_cmd = None
        pending_ip = None
        return

    wifi_safe_on()
    time.sleep(1)  # DHCP & TCP 安定待ち

    # Zero2W に SUCCESS 通知
    notify_zero2w(pending_ip)

    print("IR done:", mode)
    led.off()

    # 処理完了
    processing = False
    pending_cmd = None
    pending_ip = None

# ===== メインループ =====
while True:
    wdt.feed()

    # まず重い処理（保留コマンド）を進める
    process_pending()
    led.off()

    # 受信ループ（軽量）
    try:
        try:
            conn, addr = server.accept()
        except OSError:
            # タイムアウト → Wi-Fi 死んでたら復旧
            if not wlan.isconnected():
                ensure_wifi()
            continue

        conn.settimeout(2)

        try:
            data = conn.recv(128)
        except OSError:
            conn.close()
            continue

        if not data:
            conn.close()
            continue

        cmd_str = data.decode().strip()
        kind, mode = classify_command(cmd_str)

        # PING は即応答
        if kind == "PING":
            conn.send(b"PONG")
            conn.close()
            continue

        # IR コマンド
        if kind == "IR":
            if processing:
                # すでに1件処理中 → BUSY
                conn.send(b"BUSY")
            else:
                # 受け付けて OK を返すだけ
                pending_cmd = cmd_str
                pending_ip = addr[0]
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
