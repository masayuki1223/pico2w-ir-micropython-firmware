import subprocess
import platform
import time
import socket
from fastapi import FastAPI
from fastapi.responses import HTMLResponse

app = FastAPI()

PICO_IR_IP = "192.168.0.203"
PICO_IR_TCP_PORT = 5000

# -----------------------------
#  Ping 判定
# -----------------------------
def ping_check(ip, timeout=5):
    """
    5秒以内に ping 応答があれば True
    応答が無ければ False
    Windows / Linux 両対応
    """
    param = "-n" if platform.system().lower() == "windows" else "-c"
    start = time.time()

    while time.time() - start < timeout:
        result = subprocess.run(
            ["ping", param, "1", ip],
            capture_output=True,
            text=True
        )

        if "TTL=" in result.stdout or "ttl=" in result.stdout:
            return True

        time.sleep(0.5)

    return False


def pico_ready(ip):
    """
    Pico2W が TCP を受けられる状態か判定する。
    ping が返る → Wi-Fi ON、IR送信していない → True
    ping が返らない → Wi-Fi OFF（IR中）→ False
    """
    return ping_check(ip, timeout=5)


# -----------------------------
#  TCP 送信（判定は外側で行う）
# -----------------------------
def send_ir_tcp(cmd):
    msg = f"IR:{cmd}".encode()

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        sock.connect((PICO_IR_IP, PICO_IR_TCP_PORT))
        sock.send(msg)

        resp = sock.recv(32).decode().strip()
        sock.close()

        return resp  # "OK" / "BUSY" / "NG"

    except Exception as e:
        print("TCP error:", e)
        return "ERROR"


# -----------------------------
#  API: エアコン操作
# -----------------------------
@app.get("/aircon/send")
def aircon_send(cmd: str):

    # ① Pico2W が受信可能か判定
    if not pico_ready(PICO_IR_IP):
        return {"status": "NOT_READY", "cmd": cmd}

    # ② TCP 送信
    resp = send_ir_tcp(cmd)

    return {"status": resp, "cmd": cmd}

@app.get("/")
def index():
    return HTMLResponse(f"""
        <!doctype html>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Air conditioner operation screen</title>

        <style>
        body {{
            max-width: 500px;
            margin: auto;
            padding: 20px;
            font-family: sans-serif;
        }}

        button {{
            width: 90%;
            padding: 18px;
            font-size: 22px;
            border-radius: 12px;
            margin: 12px 0;
            border: none;
            background: #f0f0f0;
        }}
        button:active {{
            background: #ddd;
        }}
        </style>
        <h2>エアコン操作パネル</h2>
        <p id="msg" style="color: green; font-weight: bold;"></p>

        <button onclick="send('COOL25')">❄ 冷房 25℃</button><br><br>
        <button onclick="send('HEAT25')">🔥 暖房 25℃</button><br><br>
        <button onclick="send('FANLOW')">🍃 送風 微風</button><br><br>
        <button onclick="send('OFF')">⛔ 停止</button><br><br>

        <script>
        const msg = document.getElementById("msg");

        function send(cmd) {{
            msg.style.color = "black"
            msg.textContent = "送信中…";  // 送信開始表示
            fetch("/aircon/send?cmd=" + cmd)
                .then(r => r.json())
                .then(j => {{

                    if (j.status === "NOT_READY") {{
                        msg.style.color = "red";
                        msg.textContent = "Pico2W が IR送信中です。しばらくお待ちください。";
                        return;
                    }}

                    if (j.status === "BUSY") {{
                        msg.style.color = "orange";
                        msg.textContent = "Pico2W が処理中です…";
                        return;
                    }}

                    if (j.status === "ERROR") {{
                        msg.style.color = "red";
                        msg.textContent = "通信エラーが発生しました";
                        return;
                    }}
                        
                    if (j.status === "OK") {{
                        msg.style.color = "green";
                        msg.textContent = "送信完了！";
                        setTimeout(() => msg.textContent = "", 3000);
                        return;
                    }}

                }});
        }}
        </script>
    """)
