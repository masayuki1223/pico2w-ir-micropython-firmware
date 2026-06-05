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
#  Ping check
# -----------------------------
def ping_check(ip, timeout=5):
    """
    Return True if ping responds within 5 seconds.
    Works on both Windows and Linux.
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
    Determine whether Pico2W is ready to receive TCP.
    Ping OK → Wi-Fi ON, Pico2W ready → True
    Ping NG → Wi-Fi OFF or Pico2W down → False
    """
    return ping_check(ip, timeout=5)


# -----------------------------
#  TCP send (readiness is checked outside)
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
#  API: Air conditioner control
# -----------------------------
@app.get("/aircon/send")
def aircon_send(cmd: str):

    # 1. Check if Pico2W is ready
    if not pico_ready(PICO_IR_IP):
        return {"status": "NOT_READY", "cmd": cmd}

    # 2. TCP send
    resp = send_ir_tcp(cmd)

    return {"status": resp, "cmd": cmd}


@app.get("/")
def index():
    return HTMLResponse(f"""
        <!doctype html>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Air Conditioner Control Panel</title>

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

        <h2>Air Conditioner Control Panel</h2>
        <p id="msg" style="color: green; font-weight: bold;"></p>

        <button onclick="send('DRY25')">💧 Dry Mode 25°C</button><br><br>
        <button onclick="send('COOL25')">❄ Cool 25°C</button><br><br>
        <button onclick="send('HEAT25')">🔥 Heat 25°C</button><br><br>
        <button onclick="send('FANLOW')">🍃 Fan (Low)</button><br><br>
        <button onclick="send('OFF')">⛔ Power Off</button><br><br>

        <script>
        const msg = document.getElementById("msg");

        function send(cmd) {{
            msg.style.color = "black";
            msg.textContent = "Sending…";  // Sending start

            fetch("/aircon/send?cmd=" + cmd)
                .then(r => r.json())
                .then(j => {{

                    if (j.status === "NOT_READY") {{
                        msg.style.color = "red";
                        msg.textContent = "No response from Pico2W.";
                        return;
                    }}

                    if (j.status === "BUSY") {{
                        msg.style.color = "orange";
                        msg.textContent = "Pico2W is busy…";
                        return;
                    }}

                    if (j.status === "ERROR") {{
                        msg.style.color = "red";
                        msg.textContent = "Communication error occurred.";
                        return;
                    }}

                    if (j.status === "OK") {{
                        msg.style.color = "green";
                        msg.textContent = "Command sent successfully!";
                        setTimeout(() => msg.textContent = "", 3000);
                        return;
                    }}

                }});
        }}
        </script>
    """)
