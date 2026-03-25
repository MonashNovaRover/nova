#!/home/nova/Builds/master/bin/python3

import jcan


import time
import sys


# if they are not back by this time give up
TIMEOUT = 2000

# 10 at 20hz is 0.5 seconds?
TELEM_COUNT_FOR_ALIVE = 80

args = sys.argv[:]

if len(args) == 1:
    print(f"usage (eg reset j1, j3, j4): {args[0]} 1 3 4")
    exit(1)

args.pop(0)

joints = list(map(int, args))

bus = jcan.Bus()

bus.open("can1")

for j in joints:
    # get zero
    print("requesting zero from", j)
    bus.send(jcan.Frame(0x009 | j << 4, [0xf]))

    get_feedback_bus = jcan.Bus();
    get_feedback_bus.set_id_filter([0x409 | j << 4])
    get_feedback_bus.open("can1")
    zero_position = 0

    while True:
        recieved_frame = get_feedback_bus.receive_with_timeout(TIMEOUT)
        if recieved_frame == None:
            print(f"Didn't recieve frame for joint {j} before timeout!")
            quit()
        var_idx = recieved_frame.data[0]
        if var_idx == 0xf:
            # zero offset
            j = (recieved_frame.id & 0x0f0) >> 4
            print(j, "saving zero of ", recieved_frame.data[1:], "and resetting")
            zero_position = recieved_frame.data[1:]

            # got zero, now reset!
            bus.send(jcan.Frame(0x00B | j<<4, []))
            break

    time.sleep(2)

    telem_bus = jcan.Bus();
    telem_bus.set_id_filter([0x401 | j << 4])
    telem_bus.open("can1")
    telem_msgs_since_reset = 0
    while telem_msgs_since_reset <= TELEM_COUNT_FOR_ALIVE:
        recieved_frame = telem_bus.receive_with_timeout(TIMEOUT)
        print(recieved_frame.data)
        if recieved_frame == None:
            print(f"Didn't recieve telemetry for joint {j}  before timeout!")
            quit()

        telem_msgs_since_reset += 1

    print(j, "is alive again, restoring zero...")
    # 20hz telem  iirc, so half a second?
    bus.send(jcan.Frame(0x00A | j << 4, [0xf]+zero_position))

