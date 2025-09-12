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

    def registerAttr(self, name, getter, width, height=1, units=""):
        self.attrs.append(Device.Attribute(name, getter, width,height=height, units=units))




