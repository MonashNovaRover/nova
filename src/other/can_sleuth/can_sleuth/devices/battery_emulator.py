'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

Nova Battery Emulator

See also: https://www.notion.so/MNR-CANBUS-Standards-9dc47508ed3e4dfda2aa9ae97fe1ad54

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from . import candevice
from . import device
from . import battery

import struct

class BatteryEmulator(candevice.CanDevice):
    """Battery Emulator
    """
    # TODO: 4B3 and 0B3 for the shutdown handshake

    def __init__(self, name="Battery", idNumber=0xB, interface="can0"):
        """Create the Battery sniffer

        :param name: Display name of the battery
        :param idNumber: the id of this battery
        :param interface: the name of the canbus this blcmd is on
        """

        # we match only the shut down confirmed message from the jetson.
        self.id = idNumber
        self.state = 0
        super().__init__(name, interface , canIdMask=0xfff, canIdMatch=(self.id<<4)|0x3);


    def update(self):
        self.state = (self.state+1)%100

        for telemId in battery.Battery.telemetry:
            bytes_ = b''
            for i, (name, fmt, units, toReadable) in enumerate(battery.Battery.telemetry[telemId]):
                val = (10+i*100+(self.state*i))
                bytes_ += struct.pack(fmt, val)

            self.sendFrame(0x400| self.id << 4 | telemId, list(bytes_))




