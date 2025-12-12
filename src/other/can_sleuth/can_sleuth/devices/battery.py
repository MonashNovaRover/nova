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


    # all 16 bit integers
    telemetry = {
            0: (
                ("V(Cell 1)", ">H", "mV", str),
                ("V(Cell 2)", ">H", "mV", str),
                ("V(Cell 3)", ">H", "mV", str),
                ("V(Cell 4)", ">H", "mV", str)
            ),
            1: (
                ("V(Cell 5)", ">H", "mV", str),
                ("V(Cell 6)", ">H", "mV", str),
                ("V(Cell 7)", ">H", "mV", str),
                ("V(Cell 8)", ">H", "mV", str)
            ),
            2: (
                ("I(Peak)", ">h", "mA", lambda x: str(x*10)), # centi-amps to mili-amps
                ("V(Peak)", ">H", "mV", str)
            )
            }

    # TODO: 4B3 and 0B3 for the shutdown handshake

    def __init__(self, name="Battery", idNumber=0xB, interface="can0"):
        """Create the Battery sniffer

        :param name: Display name of the battery
        :param idNumber: the id of this battery
        :param interface: the name of the canbus this blcmd is on
        """

        # we match both commands to the battery and telemetry/errors coming back
        self.id = idNumber
        super().__init__(name, interface , canIdMask=0xbf0, canIdMatch=self.id<<4);

        for telemIdNumber in Battery.telemetry:
            fields = []
            for name, fmt, units, toReadable in Battery.telemetry[telemIdNumber]:
                fields.append(device.Device.SimpleBytesAttribute(fmt, name, toReadable, units))

            # I can still hear my FIT2099 TA telling me off
            candevice.CanDevice.SimpleCANMessageHandler(
                    self, 0x400 | (self.id << 4) | telemIdNumber, fields
                    )

    def update(self):
        pass


