import ir
import time

ir.init()  # 初期化

print("IR receive test: ボタンを押してください")

while True:
    pulses = ir.receive()
    if pulses:
        print("RAW:", pulses)
        print("len =", len(pulses))
        break
    time.sleep(0.05)
