'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

Helper functions for creating all devices in payloads/
systems.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from can_sleuth.devices import blcmd
from can_sleuth.devices import blcmd_emulator
from can_sleuth.devices import led as LED #gotta rename some things

# Interface specifies the bus (usually a canbus) that the system/payload is connected to,
# emulate indicates if in addition to tracing the state of the system/payload, if we should
# also be emulating the system/payload itself.

def taipan_spherical(interface="can1", emulate=False):
    """The Taipan Arm Payload with its exciting four bar linkage and spherical wrist.
    """
    devices = [
            blcmd.BLCMD(f"J{x}", x, interface) for x in range(1,6+1)
            ]

    if emulate:
        devices += [
            blcmd_emulator.BLCMDEmulator(f"J{x}", x, interface) for x in range(1,6+1)
            ]

    return devices

def drive25_26(interface="can0", emulate=False):
    """
    "I drive" - Ken Carson Jr from the Barbie movie

    I don't think chassis has a cool name for this, they just call every drive iteration
    "new drive".
    """
    names = {
            0x1: "FLD",
            0x2: "BLD",
            0x3: "BRD",
            0x4: "FRD",
            0x5: "FLP",
            0x6: "BLP",
            0x7: "BRP",
            0x8: "FRP"
            }

    devices = []

    for id_ in names.keys():
        devices.append(blcmd.BLCMD(names[id_], id_, interface))
    if emulate:
        for id_ in names.keys():
            hasResolver = names[id_][2] == "P" # only pivots have resolvers/abcoder
            devices.append(blcmd_emulator.BLCMDEmulator(names[id_], id_, interface, hasResolver=hasResolver))
    return devices

def led(interface = "can0", emulate=False):
    devices = [LED.LEDStrip("led", interface)]
    return devices

# List of everything for help message:
allSystems = {
        "drive": drive25_26,
        "taipan": taipan_spherical,
        "drive25_26": drive25_26,
        "taipan_spherical": taipan_spherical,
        "led": led
        }

