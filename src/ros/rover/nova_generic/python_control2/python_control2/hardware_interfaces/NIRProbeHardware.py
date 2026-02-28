"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Hardware interface for NIR Probe.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - <joint>/effort  [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Yahya Muayyiduddin
CREATION:       01/02/26
EDITED:         01/02/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import jcan

from ..controller_manager.Interface import Interface, InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts
from struct import pack
from teleop_python_utils import Inputs, EventCollection


class NIRProbeHardware(HardwareInterface):
    effort_cmd: Interface
    can_id: int
    hardware: str
 

    def __init__(self, contexts: Contexts,
                 hardware: str="NIRProbeHardware",
                 can_id: int=0x0E9,
                ):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]
        self.can_id = self.declare_parameter("can_id", can_id).value
        self.hardware = self.declare_parameter("hardware", hardware).value

        if EventCollection in contexts:
            events = contexts[EventCollection]
            # Listen for <hardware>/take reading events to send CAN command
            events[f"{self.hardware}/take_reading"].add_callback(self.take_reading)
        else:
            self.logger.error("Could not find EventCollection in the python control contexts.")


    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        """ Used to set up your HardwareInterface. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        # Update params
        return True

    def on_read(self, now: float, period: float):
        """ Called to read values from hardware, and put them into stored state interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass

    def on_write(self, now: float, period: float):
        """ Called to write to hardware using values stored in command interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass

    def take_reading(self):
        self.bus.send(self.construct_frame())

    def construct_frame(self) -> jcan.Frame:
        """ Construct the jcan Frame based on current command interface """
     
        # Create and return the frame
        frame = jcan.Frame(id=self.can_id, data=[])

        return frame
