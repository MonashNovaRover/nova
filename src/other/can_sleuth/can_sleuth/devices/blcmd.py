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
        self.lastCommand.value = BLCMD.commands[commandNumber]["name"] \
                + str(BLCMD.commands[commandNumber]["fmt"](frame.data))


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

    errorLevels = {
            0: "ERR ",
            1: "WARN",
            2: "INFO",
            3: "GATE"
            }


    def __init__(self, name, idNumber, interface, multiturn=False):
        """Create the BLCMD sniffer

        :param name: Display name of the blcmd
        :param idNumber: the id of this blcmd
        :param interface: the name of the canbus this blcmd is on
        """

        # we match both commands to the blcmd and telemetry/errors coming back
        self.id = idNumber

        if multiturn:
            telem3 = (
                ("resolverPosition", ">h", "°", lambda x: f"{x*360*4/0x10000:+03.2f}"),
                ("resolverTurns", ">h", "", None)
            )
        else:
            telem3 = (
                ("resolverPosition", ">h", "°", lambda x: f"{x*360/0x10000:+03.2f}"),
                ("resolverVelocity", ">h", "", None)
            )

        # telemetry from the blcmd
        telemetry = {
                (0x400 | self.id << 4): (
                    ("err", "BB", "", lambda x:
                     f"{self.errorLevels.get(x[0],str(x[0]))} {self.errorCodes.get(x[1],str(x[1]))}"
                     ),
                ),
                (0x401 | self.id << 4): (
                    ("velocity", ">H", "", None),
                    ("Qcurrent", ">H", "", None),
                ),
                (0x402 | self.id << 4): (
                    ("interval", ">H", "", None),
                    ("Dcurrent", ">H", "", None),
                ),
                (0x403 | self.id << 4): telem3,
                (0x404 | self.id << 4): (
                    ("power", ">H", "", None),
                    ("voltage", ">H", "", None),
                    ("temperature", ">H", "", None),
                    ("current", ">H", "", None),
                ),
            }

        super().__init__(name, interface , canIdMask=0xbf0, canIdMatch=self.id<<4, telemetry=telemetry);

        # Commands from the computer
        for cmd in BLCMD.commands.keys():
            canId = (self.id<<4) | cmd
            self.addCallback(canId, self._cmdCb)



        # TODO: trace get/set configuration messages on can


        self.lastCommand = self.registerAttr("msg", "", 15)

        # update this value as needed
        EXPECTED_MOST_NUMBER_OF_PROBLEMS_AT_ONCE = 2

        self._diagnosis = self.registerAttr(
                "diagnosis", "-", 10,
                height=EXPECTED_MOST_NUMBER_OF_PROBLEMS_AT_ONCE
                )

        # cache pointers to these so we don't search for them every update
        self._velocity = self.getAttrByName("velocity")
        self._err = self.getAttrByName("err")

    def update(self):
        diagnosis = []

        if self._velocity.raw in ("0x7f81", "0x7f7f"):
            diagnosis.append("ENCODER DC'd")

        if self._err.raw[4:6] in ("06", "07"): # "GATE_DRIVER_SPI", "MA302_SPI"
            diagnosis.append("CHECK FUSE")

        self._diagnosis.value = "\n".join(diagnosis)
        if diagnosis:
            self._diagnosis.priority = self.Attribute.Priority.ERROR
        else:
            self._diagnosis.priority = self.Attribute.Priority.INFO


