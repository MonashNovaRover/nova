'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

Nova Battery Tracer

See also: https://www.notion.so/MNR-CANBUS-Standards-9dc47508ed3e4dfda2aa9ae97fe1ad54

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from . import candevice
from . import device

class Battery(candevice.CanDevice):
    """Battery Tracer, shows the state of the battery on the canbus
    """

    def telemetry(self, idNumber):
        strPadded = lambda width: lambda x: str(x).ljust(width)
        return {
                (0x400 | idNumber << 4): (
                    ("V(Cell 1)", ">H", "mV", strPadded(4)),
                    ("V(Cell 2)", ">H", "mV", strPadded(4)),
                    ("V(Cell 3)", ">H", "mV", strPadded(4)),
                    ("V(Cell 4)", ">H", "mV", strPadded(4))
                ),
                (0x401 | idNumber << 4): (
                    ("V(Cell 5)", ">H", "mV", strPadded(4)),
                    ("V(Cell 6)", ">H", "mV", strPadded(4)),
                    ("V(Cell 7)", ">H", "mV", strPadded(4)),
                    ("V(Cell 8)", ">H", "mV", strPadded(4))
                ),
                (0x402 | idNumber << 4): (
                    ("I(Peak)", ">h", "mA", lambda x: strPadded(4)(x*10)), # centi-amps to mili-amps
                    ("V(Peak)", ">H", "mV", strPadded(4))
                )
                }

    # TODO: 4B3 and 0B3 for the shutdown handshake

    def __init__(self, name="Battery", idNumber=0xB, interface="can0"):
        """Create the Battery sniffer

        :param name: Display name of the battery
        :param idNumber: the can id of this battery
        :param interface: the name of the canbus this battery is on
        """

        # we match telemetry from the battery, and the shutdown handshake ids
        super().__init__(name, interface , canIdMask=0xbf0, canIdMatch=idNumber<<4, telemetry=self.telemetry(idNumber))


    def update(self):
        pass


