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
from enum import Enum, auto
from typing import Union

import time

class Device(abc.ABC):
    def __init__(self, name, aliveTimeout=1):
        self.name = name
        self._width = 0
        self.attrs = []

        self._aliveTimeout=aliveTimeout
        self.lastSeenAlive = 0

    def connected(self):
        if self._aliveTimeout is -1:
            return True
        return time.time() - self.lastSeenAlive < self._aliveTimeout

    def alive(self):
        """Call this when you get a message from the device
        """
        self.lastSeenAlive = time.time()

    @dataclass
    class Attribute:

        class Priority(Enum):
            FATAL = 0
            ERROR = auto()
            WARN = auto()
            INFO = auto()
            DEBUG = auto()

        name: str
        # value is ideally si units or human readable
        _value: object # function returning string OR string
        width: int
        # raw should be similar to hex bytes or a number unprocessed
        raw: object = lambda: None # function returning string OR raw data format
        height: int = 1
        units: str = ""
        priority: Priority = Priority.INFO # can change at runtime

        @property
        def value(self):
            if callable(self._value):
                return self._value()
            else:
                return self._value
        @value.setter
        def value(self, value):
            self._value = value

        @property
        def raw(self):
            if callable(self._raw):
                return self._raw()
            else:
                return self._raw
        @raw.setter
        def raw(self, value):
            self._raw = value

    class SimpleBytesAttribute(Attribute):
        def __init__(self, name, bytesFmt, units, toHumanReadable):
            """Helper for making attributes where the data is in a binary format, unpacked and then converted to human units.

            bytesFmt: str, the meaning of these is explained here https://docs.python.org/3/library/struct.html#format-strings
            name: str, the attribute name
            toHumanReadable: converter function from unpacked value to human readable value or none if only raw hex works
            units str: units for this attribute
            """

            self._bytesFmt = bytesFmt

            self._toHumanReadable = toHumanReadable # function returning list of strings?

            self._byteLength = struct.calcsize(self._bytesFmt)

            self.updateBytesValue(bytes(self._byteLength)) # all zeros

            exampleOutput = self._getValue()
            if isinstance(exampleOutput, str):
                outputHeight = 1
                outputWidth = len(exampleOutput)
            else:
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
        :param getter: function that returns the value of this attribute as string OR value as string
        :param width: maximum width of the attribute's value in characters (not including units)
        :param height: maximum number of lines this attribute value can take
        :param units: the units for this attribute as string
        """
        attr = Device.Attribute(name, getter, width,height=height, units=units)
        self.attrs.append(attr)
        return attr
    
    def getAttrByName(self, name):
        f = filter(lambda x: name == x.name, self.attrs)
        # TODO: complain if there are two attrs of same name?
        return next(f)

