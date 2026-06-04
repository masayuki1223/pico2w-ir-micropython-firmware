import network
import socket
import time
import gc
import ir
from config import SSID, PASS
from pulses import HEAT_25, COOL_25, DRY_25, FAN_LOW, POWER_OFF
from machine import WDT, Pin

# ===== 設定 =====
STATIC_IP = ("192.168.0.203", "255.255.255.0", "192.168.0.1", "192.168.0.1")
PORT = 5000

wdt = WDT(timeout=8000)
wlan = network.WLAN(network.STA_IF)

# --- GPIO 設定 ---
btn_recv = Pin(8, Pin.IN, Pin.PULL_UP)
btn_send = Pin(9, Pin.IN, Pin.PULL_UP)

led_green  = Pin(3, Pin.OUT)     # 未使用
led_bule   = Pin(7, Pin.OUT)     # 未使用
led_white  = Pin(6, Pin.OUT)     # Wifi定期リフレッシュ中点灯
led_yellow = Pin(28, Pin.OUT)    # Wifiエラー点滅
led_pico   = Pin("LED", Pin.OUT) # IR送信動作中点灯

led_green.off()
led_bule.off()
led_white.off()
led_yellow.off()
led_pico.off()

ir.init(10, 2)

# 処理中フラグ & 保留コマンド
processing = False
pending_cmd = None
last_ir = 0

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

# --- サーバ停止 ---
def stop_server(server):
    try:
        server.close()
    except:
        pass
    gc.collect()
    time.sleep(0.2)

# --- Wi-Fi リフレッシュ ---
def wifi_refresh(wlan):
    wlan.active(False)
    time.sleep(1.2)  # ★ Pico2Wは1秒以上待たないと不安定
    wdt.feed()  # ★ 安全のため追加
    wlan.active(True)
    wlan.config(pm=0xa11140)
    time.sleep_ms(200)
    wlan.ifconfig(STATIC_IP) 

    wlan.connect(SSID, PASS)
    for _ in range(30):
        if wlan.isconnected():
            break
        time.sleep(0.1)
        wdt.feed()

# --- 定期リフレッシュ ---
def periodic_refresh(server, wlan):
    print("=== PERIODIC WIFI REFRESH ===")
    stop_server(server)
    wifi_refresh(wlan)
    new_server = start_server()
    print("=== REFRESH DONE ===")
    return new_server

# --- Wifiが落ちた時の処理 ---
def wifi_error_recovery():
    print("Wi-Fi reconnect failed → blinking yellow LED and retrying forever")

    while True:
        # LED 点滅
        led_yellow.on()
        time.sleep(0.2)
        led_yellow.off()
        time.sleep(0.2)

        wdt.feed()  # ★ WDT を止めない

        # Wi-Fi OFF → ON（Pico2Wはこれが重要）
        wlan.active(False)
        time.sleep(1.2)
        wlan.active(True)
        wlan.config(pm=0xa11140)
        time.sleep_ms(200)
        wlan.ifconfig(STATIC_IP)

        wlan.connect(SSID, PASS)

        # 最大3秒待つ
        for _ in range(30):
            if wlan.isconnected():
                print("Wi-Fi recovered:", wlan.ifconfig()[0])
                led_yellow.off()
                return  # ★ 復帰したらメインループへ戻る
            time.sleep(0.1)
            wdt.feed()

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

# ===== コマンド分類 =====
def classify_command(cmd):
    if cmd.startswith("IR:"):
        return cmd[3:]
    return None

# ===== IR 実行ルーチン =====
def process_pending():
    global processing, pending_cmd, last_ir

    if not processing or pending_cmd is None:
        return

    mode = classify_command(pending_cmd)

    if not mode:
        # 想定外だが一応クリア
        processing = False
        pending_cmd = None
        return

    print("IR start:", mode)
    led_pico.on()

    if mode == "HEAT25":
        ir.send(HEAT_25)
    elif mode == "COOL25":
        ir.send(COOL_25)
    elif mode == "DRY25":
        ir.send(DRY_25)
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
    last_ir = time.time()
    processing = False
    pending_cmd = None

# ===== メインループ =====
last_refresh = time.time()
REFRESH_INTERVAL = 120 * 60  # ★ 2時間ごと（30〜120分が安全）

while True:
    wdt.feed()
    led_pico.off()

    # IR 実行ルーチン
    process_pending()

    #定期リフレッシュ
    now = time.time()
    if (not processing 
        and now - last_refresh > REFRESH_INTERVAL
        and now - last_ir > 2):
        led_white.on()
        server = periodic_refresh(server, wlan)
        wdt.feed()
        led_white.off()
        last_refresh = now

    # 受信ループ
    try:
        try:
            conn, addr = server.accept()
        except OSError:
            # タイムアウト → Wi-Fi 死んでたら復旧
            if not wlan.isconnected():
                wlan.active(False)
                time.sleep(1.2)
                ok = ensure_wifi()
                if not ok:
                    wifi_error_recovery()  # ★ 点滅しながら永遠に再接続
                continue
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
                # 受け付けて OK を返す
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
