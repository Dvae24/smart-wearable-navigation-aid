"""Save the device's Serial output to a timestamped file during test runs.

  pip install pyserial
  python tools/serial_logger.py --port COM5            (Windows)
  python tools/serial_logger.py --port /dev/ttyUSB0    (Linux)

Type a command + Enter to send it (d = dump hazard log, s = status, r = recalibrate).
Ctrl+C to stop. Output goes to data/raw/<timestamp>.log
"""
import argparse
import threading
import time
from pathlib import Path

import serial  # pyserial

ap = argparse.ArgumentParser()
ap.add_argument("--port", required=True)
ap.add_argument("--baud", type=int, default=115200)
a = ap.parse_args()

out = Path(__file__).resolve().parents[1] / "data" / "raw"
out.mkdir(parents=True, exist_ok=True)
path = out / time.strftime("%Y%m%d-%H%M%S.log")
ser = serial.Serial(a.port, a.baud, timeout=0.2)


def send():
    while True:
        ser.write((input() + "\n").encode())


threading.Thread(target=send, daemon=True).start()
print(f"logging to {path}  (Ctrl+C to stop)")
with open(path, "w", encoding="utf-8") as f:
    try:
        while True:
            line = ser.readline().decode(errors="replace")
            if line:
                print(line, end="")
                f.write(f"{time.time():.3f}\t{line}")
                f.flush()
    except KeyboardInterrupt:
        print(f"\nsaved {path}")
