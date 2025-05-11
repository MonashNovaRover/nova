import can
import time

# TODO: circular linked list?
class Node:
    def __init__(self, val):
        self.next = None
        self.val = val
        self.prev = None

class BusArbitrator:
    def __init__(self, bus="can1", blcmds=(1,2,3,4,5,6)):
        self.tokenLocation = 0
        self.blcmds = blcmds
        self.aliveBlcmds = []
        self.nextDict = {
                0: 0,
                1: 2,
                2: 3,
                3: 4,
                4: 5,
                5: 6,
                6: 1
                }

        
        self.lastPassTime = time.time()
        filters = [
                {"can_id": 0x00F, "can_mask": 0xf0f},
                {"can_id": 0x401, "can_mask": 0xf0f},
                ]

        self.bus = can.Bus(
                interface="socketcan", channel=bus,
                can_filters=filters
                )
        self.notifier = can.Notifier(self.bus, [self.on_message])
        #self.bus.set_id_filter(tokenPassCanIds + velFeedbackCanIds)
        # TODO: telem cb

    def loop(self):
        # check if the token is getting passed
        #   if stuck remove missing blcmd and pass token
        if time.time() - self.lastPassTime > 1:
            print("timeout, skipping")
            # TODO make whoever gave the token to the current holder skip the current holder
            self.giveToken(self.nextDict[self.tokenLocation])


        pass



    def on_message(self, message):
        msgType = message.arbitration_id & 0xf

        match msgType:
            case 0xF:
                self.tokenPassCb(message)
            case 0x1:
                self.velTelemCb(message)

    def velTelemCb(self,message):
        sourceId = (message.arbitration_id & 0x0f0) >> 4
        print(f"{sourceId} is alive")

    def giveToken(self,id_):
        # give blcmd with id permission to use resolver
        self.tokenLocation = id_

        # cansend (0$F#)
        msg = can.Message(
                arbitration_id=0x00F&(id_<<4),
                data = b'',
                dlc = 0
                )
        self.bus.send(msg)
        self.lastPassTime = time.time()
        print(f"gave token to {id_}")

    def setNext(self, currId, nextId):
        # cansend 0{curr}A#0F0{next}
        msg = can.Message(
                arbitration_id=0x00A&(currId<<4),
                data = b'\x0f'+nextId.to_bytes(),
                dlc = 2
                )
        self.bus.send(msg)
        print(f"told {currId} next is {nextId}")

    def tokenPassCb(self, frame):
        nextId  = (frame.arbitration_id >> 4) & 0xf
        print(hex(frame.arbitration_id), nextId)
        if nextId != self.nextDict[self.tokenLocation]:
            print("mismatch") # TODO flush, there is a duplicate token?
        print(f"{self.tokenLocation} passed token to {nextId}")
        self.tokenLocation = nextId
        self.lastPassTime = time.time()


if __name__ == "__main__":
    busArb = BusArbitrator()
    busArb.giveToken(1)
    while True:
        time.sleep(0.01)
        busArb.loop()

