import jcan
import time
import random

good_ids = [0x511, 0x512, 0x513, 0x514, 0x515, 0x516, 0x518, 0x531, 0x541, 0x5F2, 0x5F4]
dodgy_ids = [0x5F1, 0x521, 0x5F2]

bus = jcan.Bus()    

def main():
    bus.open("vcan0")

    while True:
        frame = bus.receive()
        if frame.id != 0x500:
            continue

        probe_number = frame.data[0]
        print(probe_number)
    
        for can_id in good_ids:
            response_frame = jcan.Frame(can_id, [0, 1, probe_number])
            print(response_frame)
            bus.send(response_frame)

        for can_id in dodgy_ids:
            if random.random() > 0.7:
                response_frame = jcan.Frame(can_id, [0, 1, probe_number])
                print(response_frame)
                bus.send(response_frame)

if __name__ == "__main__":
    main()
