'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Can Sleuth / Simulator

Output Abstract class

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EDITED BY: Orlando Chamberlain
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
import abc
from typing import List

from devices import device

class Output(abc.ABC):
    """An output that does something with the state of all devices in the system
    """
    @abc.abstractmethod
    def update(self, devices: List[device.Device]):
        """To be run every time we want the output to output the current state of devices
        to the terminal/a file/whatever this output is.

        :param devices: all the devices in the system
        """
        pass
