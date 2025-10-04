#!/home/nova/Builds/master/bin/python
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

Main script for the can sleuth/simulator. Use -h
for help.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

import sys

if __name__ == "__main__":
    # ensure we can run this from the git tree
    import os
    sys.path.append(os.path.join(os.path.dirname(__file__), ".."))

from can_sleuth import manager
from can_sleuth import systems

def print_help():
    args = sys.argv
    print(f"usage: {args[0]} [[-e | --emulate] ["+"{-i | --interface} INTERFACE ] PAYLOAD]")
    print()
    print("Available Payloads/Systems:", *systems.allSystems.keys())
    print()
    print(f"E.g. to emulate+trace taipan:       `{args[0]} -e taipan`")
    print(f"     to trace drive on can2:        `{args[0]} --interface can2 drive`")
    print(f"     to emulate+trace drive+taipan: `{args[0]} -e drive -e taipan`")

    #TODO: when we have multiple outputs, document that here too!


def main():
    args = sys.argv.copy()

    if "-h" in args or "--help" in args:
        print_help()
        exit()

    interface = None
    emulate = False
    devices = []


    args.pop(0)
    if not args:
        print("Too few arguments!")
        print_help()
        exit(1)

    # There is probably a library for this...
    while args:
        arg = args.pop(0)
        if arg in ("-e", "--emulate"):
            emulate = True
        elif arg in ("-i", "--interface"):
            if not args:
                print("you didn't specify the interface after doing", arg)
                print_help()
                exit(1)
            interface = args.pop(0)
        else:
            if arg not in systems.allSystems:
                print(f"{arg} is not a recognised system.")
                print_help()
                exit(1)

            if interface is None:
                devices += systems.allSystems[arg](emulate=emulate)
            else:
                devices += systems.allSystems[arg](emulate=emulate, interface=interface)

            interface = None
            emulate = False

    if interface is not None or emulate:
        print("you specified the interface or to emulate, but not the payload for that to apply to!")
        print_help()
        exit(1)
    
    manager_ = manager.Manager(devices)
    
    while True:
        manager_.spin()

if __name__ == "__main__":
    main()
