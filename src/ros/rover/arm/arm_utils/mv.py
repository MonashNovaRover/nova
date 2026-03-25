#!/home/nova/Builds/master/bin/python3

import jcan


import time
import sys


# if they are not back by this time give up
TIMEOUT = 2000


args = sys.argv[:]

j = int(args[1])
delta = int(args[2],16)
assert ("0x" in args[2]), "must be in hex"



bus = jcan.Bus()

bus.open("can1")

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
        print(j, "saving zero of ", recieved_frame.data[1:])
        zero_position = recieved_frame.data[1:]

        big = recieved_frame.data[1]
        little = recieved_frame.data[2]
        offset = (big << 8) + little
        offset += delta
        print(delta, hex(big), hex(little))
        print(offset, hex(offset))
        new_offset = int.to_bytes(offset, 2, byteorder="big", signed=True)            
        break


print(j, "new offset", new_offset)
# 20hz telem  iirc, so half a second?
bus.send(jcan.Frame(0x00A | j << 4, [0xf, new_offset[0], new_offset[1]]))

