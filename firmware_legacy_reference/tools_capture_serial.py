import serial
import sys
import time

port = serial.Serial()
port.port = "COM8"
port.baudrate = 115200
port.timeout = 0.2
port.dtr = False  # avoid the auto-reset-on-open quirk noted in FASE0-CHECKLIST
port.rts = False
port.open()

end = time.time() + 120
with open("serial_capture.txt", "w", encoding="utf-8", errors="replace") as f:
    while time.time() < end:
        data = port.read(4096)
        if data:
            text = data.decode("utf-8", errors="replace")
            f.write(text)
            f.flush()
            sys.stdout.write(text)
            sys.stdout.flush()
port.close()
