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

