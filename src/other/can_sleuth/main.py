#!/home/nova/Builds/master/bin/python

import blcmd
import blcmd_emulator
import manager

import sys

def drive(interface="can0", emulate=False):
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
            devices.append(blcmd_emulator.BLCMDEmulator(names[id_], id_, interface))
    return devices

def taipan(interface="can1", emulate=False):
    devices = [
            blcmd.BLCMD("J1", 1, interface),
            blcmd.BLCMD("J2", 2, interface),
            blcmd.BLCMD("J3", 3, interface),
            blcmd.BLCMD("J4", 4, interface),
            blcmd.BLCMD("J5", 5, interface),
            blcmd.BLCMD("J6", 6, interface),
            ]

    if emulate:
        devices += [
            blcmd_emulator.BLCMDEmulator("J1",1,interface),
            blcmd_emulator.BLCMDEmulator("J2",2,interface),
            blcmd_emulator.BLCMDEmulator("J3",3,interface),
            blcmd_emulator.BLCMDEmulator("J4",4,interface),
            blcmd_emulator.BLCMDEmulator("J5",5,interface),
            blcmd_emulator.BLCMDEmulator("J6",6,interface)
            ]
    return devices

if __name__ == "__main__":
    #emulate = input("Do you have real blcmds connected? (Y/N): ").lower()[0] == 'n'
    #devices = taipan(emulate=emulate)
    devices=drive()
    manager = manager.Manager(devices,outputs=None)
    
    while True:
        manager.spin()

