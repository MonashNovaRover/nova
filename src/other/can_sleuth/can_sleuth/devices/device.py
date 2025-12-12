'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

Device abstract class

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

import abc
import struct
from dataclasses import dataclass

class Device(abc.ABC):
    def __init__(self, name):
        self.name = name
        self._width = 0
        self.attrs = []

    @dataclass
    class Attribute:
        # TODO: reconsider if value/raw should be functions or just values the device puts there on update()
        name: str
        value: object # function returning string
        width: int
        raw: object = lambda: None # function returning string
        height: int = 1
        units: str = ""

    class SimpleBytesAttribute(Attribute):
        def __init__(self, bytesFmt, name, toHumanReadable, units):
            """Helper for making attributes where the data is in a binary format, unpacked and then converted to human units.

            bytesFmt: str, the meaning of these is explained here https://docs.python.org/3/library/struct.html#format-strings
            name: str, the attribute name
            toHumanReadable: converter function from unpacked value to human readable value or none if only raw hex works
            units str: units for this attribute
            """

            self._bytesFmt = bytesFmt

            self._toHumanReadable = toHumanReadable # function returning list of strings?

            self._byteLength = struct.calcsize(self._bytesFmt)

            self._bytesValue: bytes = bytes(self._byteLength)
            self._unpackedValue: int = 0

            exampleOutput = self._getValue()
            outputHeight = len(exampleOutput) # how many lines of text
            outputWidth = max(map(len, exampleOutput)) # widest line of text

            self._bytesValue: bytes = None
            self._unpackedValue: int = None

            super().__init__(name, self._getValue, outputWidth, self._getRaw, outputHeight, units)

        def updateBytesValue(self, bytesValue: bytes):
            """update the binary (bytes) representation of this attribute
            """
            self._bytesValue = bytesValue
            val = struct.unpack(self._bytesFmt, self._bytesValue)
            if len(val) == 1:
                self._unpackedValue = val[0]
            else:
                self._unpackedValue = val
                

        def getByteLength(self):
            # Check how many bytes this attribute uses.
            return self._byteLength

        def _getRaw(self):
            if self._bytesValue is None:
                return ""
            return "0x"+self._bytesValue.hex()

        def _getValue(self):
            if self._unpackedValue is None:
                return ""
            if self._toHumanReadable is None:
                return self._getRaw()
            return self._toHumanReadable(self._unpackedValue)

    def getName(self):
        return self.name

    @abc.abstractmethod
    def update(self):
        """Update internal state
        """

    def spin(self):
        """to be run more frequently"""
        pass

    def registerAttr(self, name:str, getter, width:int, height:int=1, units:str=""):
        """register an attribute to this device

        :param name: the name of this attribute
        :param getter: function that returns the value of this attribute as string
        :param width: maximum width of the attribute's value in characters (not including units)
        :param height: maximum number of lines this attribute value can take
        :param units: the units for this attribute as string
        """
        self.attrs.append(Device.Attribute(name, getter, width,height=height, units=units))

