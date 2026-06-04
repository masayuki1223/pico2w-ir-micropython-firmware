import ir
import time

ir.init(10, 2)  # 初期化、送信ピン 10、受信ピン 2

print("IR receive test: ボタンを押してください")

while True:
    pulses = ir.receive()
    if pulses:
        print("RAW:", pulses)
        print("len =", len(pulses))
        break
    time.sleep(0.05)
