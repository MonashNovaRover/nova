#!/home/nova/Builds/master/bin/python3

import jcan


import time
import sys


# if they are not back by this time give up
TIMEOUT = 2

# 10 at 20hz is 0.5 seconds?
TELEM_COUNT_FOR_ALIVE = 10

args = sys.argv[:]

if len(args) == 1:
    print(f"usage (eg reset j1, j3, j4): {args[0]} 1 3 4")
    exit(1)

args.pop(0)

joints = list(map(int, args))

bus = jcan.Bus()
ids = []

zero_positions = {}
sent_reset = {}
sent_position = {}
telem_msgs_since_reset = {}
for j in joints:
    sent_reset[j] = False
    sent_position[j] = False
    telem_msgs_since_reset[j] = 0


# can't send messages in callback
send_queue = []

def get_cb(frame):
    var_idx = frame.data[0]
    if var_idx == 0xf:
        # zero offset
        j = (frame.id & 0x0f0) >> 4
        print(j, "saving zero of ", frame.data[1:], "and resetting")
        zero_positions[j] = frame.data[1:]

        # got zero, now reset!
        send_queue.append((0x00B | j<<4, []))
        sent_reset[j] = True

def telem_cb(frame):        
    j = (frame.id & 0x0f0) >> 4
    if sent_reset[j] == True:
        telem_msgs_since_reset[j] += 1

    if telem_msgs_since_reset[j] > TELEM_COUNT_FOR_ALIVE:
        print(j, "is alive again, restoring zero...")
        # 20hz telem  iirc, so half a second?
        send_queue.append((0x00A | j << 4, [0xf]+zero_positions[j]))
        sent_position[j] = True


for j in joints:
    getid = 0x409 | j << 4
    telemid = 0x401 | j << 4
    
    ids.append(getid) # get/set feedback
    ids.append(telemid) # telemetry

    bus.add_callback(getid, get_cb);
    bus.add_callback(telemid, telem_cb);


bus.set_id_filter(
        ids
        )

bus.open("can1")

for j in joints:
    # get zero
    print("requesting zero from", j)
    bus.send(jcan.Frame(0x009 | j << 4, [0xf]))
    

start = time.time()
while time.time() < start + TIMEOUT and False in sent_position.values():
    bus.spin()
    while send_queue:
        canid, data = send_queue.pop()
        bus.send(jcan.Frame(canid, data))
        print(f"bus.send(jcan.Frame(0x{canid:03x}, {data}))")
    time.sleep(0.1)

for j in joints:
    if sent_position[j] == False:
        print("ERR", j, "didn't respond?")
