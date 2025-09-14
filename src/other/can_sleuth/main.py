#!/home/nova/Builds/master/bin/python

import blcmd
import blcmd_emulator
import meta
import manager

import sys

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
    emulate = input("Do you have real blcmds connected? (Y/N): ").lower()[0] == 'n'
    devices = taipan(emulate=emulate)
    manager = manager.Manager(devices,outputs=None)
    
    while True:
        manager.spin()

