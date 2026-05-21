#!/usr/bin/env python3

from serial import Serial
import time

print("🔌 Opening serial port...")
ser = Serial(port='COM7', baudrate=115200, timeout=1)
print(f"✅ Connected\n")

try:
    while True:
        line = ser.readline()
        if line:
            print(line.decode('ascii', errors='ignore').strip())
except KeyboardInterrupt:
    print("\n🛑 Stopped")
    ser.close()