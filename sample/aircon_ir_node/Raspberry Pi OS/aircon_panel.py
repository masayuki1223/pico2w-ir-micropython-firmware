from fastapi import FastAPI, Request
from fastapi.responses import StreamingResponse
from fastapi.responses import HTMLResponse
import asyncio

app = FastAPI()

# 接続中の SSE クライアントを保持
sse_clients = []

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

        // SSE で Zero2W からの通知を受け取る
        const evt = new EventSource("/events");
        evt.onmessage = function(e) {{
            if (e.data === "SUCCESS") {{
                msg.textContent = "送信完了！";
                setTimeout(() => msg.textContent = "", 3000);
            }}
        }};

        function send(cmd) {{
            msg.textContent = "送信中…";  // 送信開始表示
            fetch("/aircon/send?cmd=" + cmd)
                .then(r => r.json())
                .then(j => {{
                    if (j.status === "BUSY") {{
                        msg.textContent = "操作中です…";
                    }}
                }});
        }}
        </script>
    """)

import socket

PICO_IR_IP = "192.168.0.203"
PICO_IR_TCP_PORT = 5000

def send_ir_tcp(cmd):
    msg = f"IR:{cmd}".encode()

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(2)
        sock.connect((PICO_IR_IP, PICO_IR_TCP_PORT))
        sock.send(msg)

        resp = sock.recv(32).decode().strip()
        sock.close()

        return resp  # "OK" or "BUSY" or "NG"

    except Exception as e:
        print("TCP error:", e)
        return "ERROR"

@app.get("/aircon/send")
def aircon_send(cmd: str):
    resp = send_ir_tcp(cmd)

    return {"status": resp, "cmd": cmd}

@app.get("/events")
async def sse_events():
    async def event_stream():
        queue = asyncio.Queue()
        sse_clients.append(queue)
        try:
            while True:
                msg = await queue.get()
                yield f"data: {msg}\n\n"
        except:
            pass
        finally:
            sse_clients.remove(queue)

    return StreamingResponse(event_stream(), media_type="text/event-stream")

@app.post("/success")
async def success_handler(request: Request):
    body = await request.body()
    print("SUCCESS from Pico2W:", body.decode())

    # 全クライアントに通知
    for q in sse_clients:
        await q.put("SUCCESS")

    return {"status": "ok"}
