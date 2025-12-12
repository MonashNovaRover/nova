'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

BrushLess Can Motor Driver (BLCMD) Tracer

See also: https://github.com/MonashNovaRover/pics/tree/master/BLCMD.X

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from . import candevice

import math

def _toHex(x):
    if x:
        ret = "0x"
        for i in x:
            ret += hex(0x100 + i)[3:]
        return ret
    return ""

class BLCMD(candevice.CanDevice):
    """BLCMD Tracer, shows the state of the blcmd on the canbus
    """
    commands = {
            0x0: {"name": "allStop",        "fmt": _toHex},
            0x1: {"name": "twitchFw",       "fmt": _toHex},
            0x2: {"name": "twitchBk",       "fmt": _toHex},
            0x3: {"name": "Vel: ",          "fmt": _toHex},
            0x4: {"name": "Pos: ",          "fmt": _toHex},
            0x5: {"name": "Current: ",      "fmt": _toHex},
            0x6: {"name": "OpenLoop: ",     "fmt": _toHex},
            0x7: {"name": "homeRotor",      "fmt": _toHex},
            0x8: {"name": "zero",           "fmt": _toHex},
            0x9: {"name": "getConf: ",      "fmt": _toHex},
            0xa: {"name": "setConf: ",      "fmt": _toHex},
            0xb: {"name": "rst",            "fmt": _toHex},
            0xc: {"name": "rstResolver",    "fmt": _toHex},
            0xd: {"name": "readGateFault",  "fmt": _toHex}
            }

    def _cmdCb(self,frame):
        """process commands on can
        """
        commandNumber = frame.id&0xf
        self.lastCommand = BLCMD.commands[commandNumber]["name"] \
                + str(BLCMD.commands[commandNumber]["fmt"](frame.data))

    telemetry = {
            1: (
                ("velocity", ">H", "", None),
                ("Qcurrent", ">H", "", None)
            ),
            2: (
                ("interval", ">H", "", None),
                ("Dcurrent", ">H", "", None)
            ),
            3: (
                ("resolverPosition", ">h", "°", lambda x: f"{x*360/0x10000:+03.2f}"),
                ("resolverTurns", ">h", "", None) # only correct for multiturn!
            ),
            4: (
                ("power", ">H", "", None),
                ("voltage", ">H", "", None),
                ("temperature", ">H", "", None),
                ("current", ">H", "", None),
            ),
            }

    errorCodes = {
           0: "NO_STEPS",
           1: "MAGNETIC_ENCODER",
            2: "RESOLVER",
            3: "RESOLVER_CHECKSUM",
            4: "POSITION_COMMAND",
            5: "GATE_DRIVER_FAULT",
            6: "GATE_DRIVER_SPI",
            7: "MA302_SPI",
            8: "ROTOR_HOME",
            9: "RESOLVER_ZERO",
            10:"STALL_TRIGGERED",
            11:"OVER_SPEED",
    }
    def _errCb(self, frame):
        """process errors on can
        """
        if (len(frame.data) != 2):
            return

        match frame.data[0]:
            case 0:
                level = "ERR "
            case 1:
                level = "WARN"
            case 2:
                level = "INFO"
            case 3:
                level = "GATE"

        message = self.errorCodes.get(frame.data[1], frame.data[1])
        self.error = f"{level}: {message}"

    def __init__(self, name, idNumber, interface):
        """Create the BLCMD sniffer

        :param name: Display name of the blcmd
        :param idNumber: the id of this blcmd
        :param interface: the name of the canbus this blcmd is on
        """

        # we match both commands to the blcmd and telemetry/errors coming back
        self.id = idNumber
        super().__init__(name, interface , canIdMask=0xbf0, canIdMatch=self.id<<4);

        # Commands from the computer
        for cmd in BLCMD.commands.keys():
            canId = (self.id<<4) | cmd
            self.addCallback(canId, self._cmdCb)

        # telemetry from the blcmd
        for telemIdNumber in BLCMD.telemetry:
            fields = []
            for name, fmt, units, toReadable in BLCMD.telemetry[telemIdNumber]:
                fields.append(self.SimpleBytesAttribute(fmt, name, toReadable, units))

            # I can still hear my FIT2099 TA telling me off
            self.SimpleCANMessageHandler(
                    self, 0x400 | (self.id << 4) | telemIdNumber, fields
                    )

        # TODO: trace get/set configuration messages on can

        # errors from blcmd
        self.error = None
        self.registerAttr("err", lambda: self.error, 25)
        self.addCallback(0x400 | (self.id<<4), self._errCb);

        self.lastCommand = ""
        self.registerAttr("msg", lambda: self.lastCommand, 15)

    def update(self):
        pass


