#!/home/nova/Builds/master/bin/python
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

Main script for the can sleuth/simulator.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

import manager
import systems

if __name__ == "__main__":
    devices = systems.drive() #+ systems.taipan(emulate=True)
    manager = manager.Manager(devices)
    
    while True:
        manager.spin()

