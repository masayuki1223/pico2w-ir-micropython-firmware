import socket
import network
import time
import ir
from pulses import HEAT_25, COOL_25, FAN_LOW, POWER_OFF
from machine import WDT

wdt = WDT(timeout=8000)  # 8秒

# ===== Wi-Fi 設定 =====
ssid = "xxxxxxxxxxxxxxxxxxxx"
password = "xxxxxxxxxxxxxxxxxx"
STATIC_IP = ("192.168.0.xxx", "255.255.255.0", "192.168.0.1", "192.168.0.1")# 固定IP設定

# ===== Wi-Fi インターフェース =====
wlan = network.WLAN(network.STA_IF)

def ensure_wifi():
    if not wlan.isconnected():
        print("Wi-Fi disconnected. Reconnecting...")
        wdt.feed()
        wlan.active(True)
        wlan.ifconfig(("192.168.0.203", "255.255.255.0", "192.168.0.1", "192.168.0.1"))
        wlan.connect(ssid, password)
        for _ in range(20):
            if wlan.isconnected():
                print("Reconnected:", wlan.ifconfig()[0])
                return
            time.sleep(0.5)
        print("Reconnect failed.")

# 初回接続
ensure_wifi()
ir.init()

# ===== UDP ソケット =====
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("0.0.0.0", 5005))

while True:
    wdt.feed()
    data, addr = sock.recvfrom(1024)
    cmd = data.decode().strip()
    print("cmd:", cmd)
    
    # Wi-Fi が切れていたら復帰
    if not wlan.isconnected():
        ensure_wifi()
        ir.init()  # ← ここだけ追加

    wdt.feed()

    if cmd == "HEAT25":
        ir.send(HEAT_25)
    elif cmd == "COOL25":
        ir.send(COOL_25)
    elif cmd == "FANLOW":
        ir.send(FAN_LOW)
    elif cmd == "OFF":
        ir.send(POWER_OFF)

    wdt.feed()

