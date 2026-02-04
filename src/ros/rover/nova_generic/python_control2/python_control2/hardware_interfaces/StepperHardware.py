"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Hardware interface for Stepper motors.

Can be controlled by effort or position, position
command interface must be None to use effort
control.

Can be zeroed by invoking the "<joint>/zero" event.
(found in the EventCollection context)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - <joint>/effort      [value between -1 and 1]
  - <joint>/position    [number of steps away from zero position]
STATE INTERFACES:
  - <joint>/position    [number of steps away from zero position]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Felicity Matthews
CREATION:       01/02/26
EDITED:         04/02/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import jcan
from teleop_python_utils import EventCollection

from ..controller_manager.Interface import Interface, InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts
from struct import pack


class StepperHardware(HardwareInterface):
    effort_cmd: Interface
    position_cmd: Interface
    position_state: Interface
    can_id: int
    # The name of the joint
    joint: str
    reversed: bool
    max_steps_percent: float
    max_steps_can: int

    def __init__(self, contexts: Contexts,
                 joint: str="",
                 can_id: int=0,
                 reversed: bool=False,
                 max_steps_percent: float=1.0, max_steps_can: int=0x7F):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]
        self.current_position = 0

        # Default joint name to the hardware interface name
        if len(joint) == 0:
            joint = self.name

        self.declare_parameter("joint", joint, "Name of the joint")
        self.declare_parameter("can_id", can_id, "CAN ID of the Stepper")
        self.declare_parameter("reversed", reversed, "Whether the output should be reversed")
        self.declare_parameter("max_steps_percent", max_steps_percent, "Max percentage of output to send")
        self.declare_parameter("max_steps_can", max_steps_can, "Max CAN message value that can be sent")

        # Setup zero event callback.
        if EventCollection in contexts:
            events = contexts[EventCollection]
            events[f"{self.get_parameter("joint").value}/zero"].add_callback(self.zero)
        else:
            self.logger.error("Could not find EventCollection in the python control contexts, cannot zero StepperHardware.")

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

        self.max_steps_percent = self.get_parameter("max_steps_percent").value
        self.max_steps_can = self.get_parameter("max_steps_can").value

        # Get command interfaces
        self.effort_cmd = command_interfaces[self.joint + "/effort"]
        self.position_cmd = command_interfaces[self.joint + "/position"]
        self.position_state = state_interfaces[self.joint + "/position"]

        # Validate command interface configuration
        if not self.effort_cmd:
            self.logger.warn(f"Stepper \"{self.name}\" has no populated command interfaces. "
                             f"(\"{self.joint}/effort\")")

        return True

    def zero(self):
        """ Zeros the hardware """
        self.current_position = 0

    def on_read(self, now: float, period: float):
        """ Called to read values from hardware, and put them into stored state interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        self.position_state.value = self.current_position

    def on_write(self, now: float, period: float):
        """ Called to write to hardware using values stored in command interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        # Prioritizes position control over effort.
        if self.position_cmd is not None:
            steps_to_move = self.position_to_steps(self.position_cmd.value)

            if steps_to_move > 0:
                frame = self.construct_frame(steps_to_move)
                self.bus.send(frame)
                self.current_position += steps_to_move

        elif self.effort_cmd.value != 0:
            steps_to_move = self.effort_to_steps(self.effort_cmd.value)
            frame = self.construct_frame(steps_to_move)
            self.bus.send(frame)
            self.current_position += steps_to_move

    def position_to_steps(self, position) -> int:
        """ Convert goal position to a discrete number of steps. """
        data = position - self.current_position

        # Check if the data is greater than the max value
        # If it is, set the data to the max value
        if data > self.max_steps_can:
            data = self.max_steps_can
        elif data < -self.max_steps_can:
            data = -self.max_steps_can

        data *= self.max_steps_percent

        return data

    def effort_to_steps(self, effort) -> int:
        """ Convert effort to a discrete number of steps. """
        # Set the data based on the effort, max percent value, max can value and reversed
        # Effort is directional.
        data = int(effort * self.max_steps_percent * self.max_steps_can)

        # Check if the data is greater than the max value
        # If it is, set the data to the max value
        if data > self.max_steps_can:
            data = self.max_steps_can
        elif data < -self.max_steps_can:
            data = -self.max_steps_can

        return data

    def construct_frame(self, steps_to_move) -> jcan.Frame:
        """ Construct the jcan Frame based on how many steps to move """
        if not -self.max_steps_can <= steps_to_move <= self.max_steps_percent:
            self.logger.error(f"{steps_to_move} is outside of range [{-self.max_steps_can}, {self.max_steps_percent}]")
            return jcan.Frame(id=self.can_id, data=[0])

        # # Pack the data into a list
        packed_data = list(pack('>b', int(steps_to_move * self.reversed))) # >b = big-endian signed byte (2 hex digits)

        # Create and return the frame
        frame = jcan.Frame(id=self.can_id, data=packed_data)

        return frame
