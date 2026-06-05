import ir
import time

ir.init(10, 2)  # Initialize: TX pin 10, RX pin 2

print("IR receive test: Press a button on the remote")

while True:
    pulses = ir.receive()
    if pulses:
        print("RAW:", pulses)
        print("Length =", len(pulses))
        break
    time.sleep(0.05)
