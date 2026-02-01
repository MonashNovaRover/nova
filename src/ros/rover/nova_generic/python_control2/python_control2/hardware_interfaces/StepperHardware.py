"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Hardware interface for Stepper motors.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - <joint>/effort  [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Felicity Matthews
CREATION:       01/02/26
EDITED:         01/02/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import jcan

from ..controller_manager.Interface import Interface, InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts
from struct import pack


class StepperHardware(HardwareInterface):
    effort_cmd: Interface
    can_id: int
    # The name of the joint
    joint: str
    reversed: bool
    max_effort: float
    max_effort_can: int

    def __init__(self, contexts: Contexts,
                 joint: str="",
                 can_id: int=0,
                 reversed: bool=False,
                 max_effort: float=1.0, max_effort_can: int=0x7F):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]

        # Default joint name to the hardware interface name
        if len(joint) == 0:
            joint = self.name

        self.declare_parameter("joint", joint, "Name of the joint")
        self.declare_parameter("can_id", can_id, "CAN ID of the Stepper")
        self.declare_parameter("reversed", reversed, "Whether the output should be reversed")
        self.declare_parameter("max_effort", max_effort, "Max percentage of output to send")
        self.declare_parameter("max_effort_can", max_effort_can, "Max CAN message value that can be sent")

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
        self.can_id: int = self.get_parameter("can_id").value
        self.joint: str = self.get_parameter("joint").value
        self.reversed: int = -1 if self.get_parameter("reversed").value else 1

        self.max_effort = self.get_parameter("max_effort").value
        self.max_effort_can = self.get_parameter("max_effort_can").value

        # Get command interfaces
        self.effort_cmd = command_interfaces[self.joint + "/effort"]

        # Validate command interface configuration
        if not self.effort_cmd:
            self.logger.warn(f"Stepper \"{self.name}\" has no populated command interfaces. "
                             f"(\"{self.joint}/effort\")")

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
        if self.effort_cmd.value != 0:
            frame = self.construct_frame()
            self.bus.send(frame)

    def construct_frame(self) -> jcan.Frame:
        """ Construct the jcan Frame based on current command interface """
        # Set the data based on the effort, max percent value, max can value and reversed
        # Effort is directional.
        data = int(self.effort_cmd.value * self.max_effort * self.max_effort_can * self.reversed)

        # Check if the data is greater than the max value
        # If it is, set the data to the max value
        if data > self.max_effort_can:
            data = self.max_effort_can
        elif data < -self.max_effort_can:
            data = -self.max_effort_can

        # # Pack the data into a list
        packed_data = list(pack('>b', int(data))) # >b = big-endian signed byte (2 hex digits)

        # Create and return the frame
        frame = jcan.Frame(id=self.can_id, data=packed_data)

        return frame
