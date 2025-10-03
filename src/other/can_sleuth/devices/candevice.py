'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

Can Device Abstract class, for use in implementing
emulators/tracers for hardware that uses the CAN bus

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

import jcan
from typing import List

from . import device

class CanDevice(device.Device):
    def __init__(self, name, interface, canIdList=None, canIdMask=None, canIdMatch=None):
        """Create the can device

        :param name: display name
        :param interface: the name of the canbus

        For filtering can messages, you can either give a list of can IDs to listen to
        or use a mask (only recieve messages where id&mask==match).

        Using a list:
        :param canIdList: a list of CAN IDs that you want to register callbacks for

        Using a mask:
        :param canIdMask: bitmask for the CAN IDs you want to register callbacks for
        :param canIdMatch: what you want to match
        """

        super().__init__(name)
        self.bus = jcan.Bus()

        if (canIdList is not None):
            self.bus.set_id_filter(canIdList)
        if (canIdMask is not None and canIdMatch is not None):
            self.bus.set_id_filter_mask(canIdMatch, canIdMask)

        self.bus.open(interface)

    def addCallback(self, canId, callback):
        """Add a callback function for a specific CAN ID

        :param canId: the CAN ID, eg 0x123
        :param callback: the function to be called when a message
        with the specified ID is recieved.
        """
        self.bus.add_callback(canId, callback)

    def sendFrame(self, canId: int, data: List[int]):
        """Helper function to send a CAN message.

        :param canId: the CAN ID of the outgoing message
        :param data: the data to be sent
        """
        self.bus.send(jcan.Frame(canId, data))

    def spin(self):
        """Process any incoming messages. You must run this in your spin()"""
        self.bus.spin()

