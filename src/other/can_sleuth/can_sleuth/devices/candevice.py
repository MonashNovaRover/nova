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
    def __init__(self, name, interface, canIdList=None, canIdMask=None, canIdMatch=None, telemetry={}):
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

        :param telemetry: dictionary of canIdNumber: (name, fmt, units, toReadable or None).
        fmt is for struct.unpack, units is str, toReadable is function that takes
        the output of struct.unpack and makes it human readable or none if you just
        want raw hex to be displayed.
        """

        super().__init__(name)
        self.bus = jcan.Bus()

        if (canIdList is not None):
            self.bus.set_id_filter(canIdList)
        if (canIdMask is not None and canIdMatch is not None):
            self.bus.set_id_filter_mask(canIdMatch, canIdMask)

        self.bus.open(interface)

        for telemIdNumber in telemetry:
            fields = []
            for name, fmt, units, toReadable in telemetry[telemIdNumber]:
                fields.append(self.SimpleBytesAttribute(name, fmt, units, toReadable))

            # I can still hear my FIT2099 TA telling me off
            CanDevice.SimpleCANMessageHandler(
                    self, telemIdNumber, fields
                    )

    class SimpleCANMessageHandler():
        """Helper for processing telemetry messages from can devices
        """
        def __init__(self, canDevice, canId, fields: List[device.Device.SimpleBytesAttribute]):
            """

            :param canDevice: the CanDevice that this telemetry message belongs to
            :param canId: the can id for this telemetry message
            :param fields: one attribute for every value represented by the payload of this can message
            construct these with whatever integer size and signed/unsigned and unit conversions as per
            whatever the can device does.
            """
            self._fields = fields
            for field in self._fields:
                # I am filled with regret and I can hear my FIT2099 TA shouting at me.
                canDevice.attrs.append(field)

            canDevice.addCallback(canId, self.onMsg)

        def onMsg(self, frame):
            """Callback for when we recieve a can message. We split the message's payload up and
            send each section of bytes to the respective SimpleBytesAttribute
            """
            position = 0
            for field in self._fields:
                length = field.getByteLength()
                field.updateBytesValue(
                        bytes(frame.data[position:position+length])
                    )
                position += length

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

