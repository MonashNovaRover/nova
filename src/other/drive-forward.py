#!/home/nova/Builds/master/bin/python3

# This is a script written for the 2026 ARCh Mapping and Autonomous task as a
# last resort backup in case auto stack totally fails
# By Jonathan Jia

import jcan
from time import sleep, time
from sys import argv
from struct import pack
from math import floor

default_drive_duration = 10
drive_speed = 0.076

max_speed = 0x7FFF
send_frequency = 20
pivot_zero_data = [0x39, 0x7D]

print("Running auto--...")
print("WARNING: don't run this script if launch-drive is running!")

drive_ids = [1, 2, 3, 4]
drive_multipliers = [-1, -1, 1, 1]

pivot_ids = [5, 6, 7, 8]

print("Opening can0...")
bus = jcan.Bus()
bus.open("can0")

# make wheels face forwards
decision = input("Request pivots to face wheel forward? (y/n)")
if decision.lower() == "y":
  print("Requesting pivots to face wheels forward...")
  for id in pivot_ids:
    bus.send(jcan.Frame(
      id=0x004 | (id << 4),
      data=pivot_zero_data
    ))

  # wait for pivots to rotate to final position
  sleep(5)
else:
  print("Not requesting wheels to face forward.")

# get drive duration
drive_duration = default_drive_duration
if len(argv) >= 2:
  try:
    drive_duration = float(argv[1])
    print(f"Set drive duration to: {drive_duration} seconds.")
  except:
    print(f"Failed to convert argument \"{argv[1]}\" to float; using default: {default_drive_duration} seconds")

print(f"Driving forwards for {drive_duration} ...")

# drive
end_time = time() + drive_duration
while time() <= end_time:
  for i, id in enumerate(drive_ids):
    bus.send(jcan.Frame(
      id=0x003 | (id << 4),
      data=list(pack(">h", floor(max_speed * drive_speed * drive_multipliers[i])))
    ))
  sleep(1 / send_frequency)

print("Finished driving.")
























