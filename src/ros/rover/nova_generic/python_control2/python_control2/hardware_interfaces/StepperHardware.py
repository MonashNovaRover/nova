"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Hardware interface for Stepper motors.

Can be controlled by effort or position, position
command interface must be None to use effort
control.

Position is zeroed on startup.

Can be zeroed by invoking the "<joint>/zero" event.
(found in the EventCollection context)
This will set the current positon as the "zero"
position.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - <joint>/effort      [value between -1 and 1]
  - <joint>/position    [distance from zero position]
STATE INTERFACES:
  - <joint>/position    [distance from zero position]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Felicity Matthews
CREATION:       01/02/26
EDITED:         24/02/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import jcan
from collections.abc import Callable
from teleop_python_utils import EventCollection

from ..controller_manager.Interface import Interface, InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts
from struct import pack


class Limits:
    """ Helper class for apply optional limits """
    def __init__(self, max_position, min_position, use_max_position, use_min_position):
        self.max_position = max_position
        self.min_position = min_position
        self.use_max_position = use_max_position
        self.use_min_position = use_min_position

    def limit(self, num):
        """ Limits the number based on provided limits. """
        if self.use_min_position and num < self.min_position:
            return self.min_position

        if self.use_max_position and num > self.max_position:
            return self.max_position

        return num

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
    limits: Limits

    def __init__(self, contexts: Contexts,
                 joint: str="",
                 can_id: int=0,
                 reversed: bool=False,
                 position_to_steps: Callable[float | int, int]=lambda x: x,
                 steps_to_position: Callable[int, int | float]=lambda x: x,
                 max_steps_percent: float=1.0, max_steps_can: int=0x7F,
                 max_position: float=300.0, min_position: float= 0.0,
                 use_max_position: bool=False, use_min_position: bool=False):
        """Constructor, deferred until the control manager has been spun.

        If you override this method and want to add your own arguments,
        ensure `contexts` remains the FIRST argument.

        :param contexts: A collection of dependency injection class instances accessible by class type.
        :param joint: Name of the joint. Defaults to the hardware interface name if empty.
        :param can_id: CAN ID of the stepper motor.
        :param reversed: Whether the output direction should be reversed.
        :param steps_to_position: Function used to convert stepper steps to an SI unit.
        :param position_to_steps: Function used to convert SI units to stepper steps.
        :param max_steps_percent: Maximum percentage of output that can be sent.
        :param max_steps_can: Maximum CAN message value that can be sent.
        :param max_position: Maximum allowed joint position (in joint units).
        :param min_position: Minimum allowed joint position (in joint units).
        :param use_max_position: Enable enforcement of the max_position limit.
        :param use_min_position: Enable enforcement of the min_position limit.
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

        # Setup limits
        self.declare_parameter("max_position", max_position, "Maximum allowed joint position (in joint units)")
        self.declare_parameter("min_position", min_position, "Minimum allowed joint position (in joint units)")
        self.declare_parameter("use_max_position", use_max_position, "Enable enforcement of max_position limit")
        self.declare_parameter("use_min_position", use_min_position, "Enable enforcement of min_position limit")

        # Setup conversion functions
        self.position_to_steps_conversion = position_to_steps
        self.steps_to_position_conversion = steps_to_position

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

        # Initialise limits
        self.limits = Limits(
            self.get_parameter("max_position").value,
            self.get_parameter("min_position").value,
            self.get_parameter("use_max_position").value,
            self.get_parameter("use_min_position").value
        )

        # Get command and state interfaces
        self.effort_cmd = command_interfaces[self.joint + "/effort"]
        self.position_cmd = command_interfaces[self.joint + "/position"]
        self.position_state = state_interfaces[self.joint + "/position"]

        # Validate command interface configuration
        if not self.effort_cmd and not self.position_cmd:
            self.logger.warn(f"Stepper \"{self.name}\" has no populated command interfaces. "
                             f"(\"{self.joint}/effort\", \"{self.joint}/position\")")

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
        steps_to_move = 0
        # Prioritizes position control over effort.
        if self.position_cmd is not None:
            steps_to_move = self.position_to_steps(self.position_cmd.value)

        elif self.effort_cmd.value != 0:
            steps_to_move = self.effort_to_steps(self.effort_cmd.value)

        if abs(steps_to_move) > 0:
            frame = self.construct_frame(steps_to_move)
            self.bus.send(frame)

        self.current_position += self.steps_to_position_conversion(steps_to_move)

    def position_to_steps(self, position) -> int:
        """ Convert goal position to a discrete number of steps. """
        desired_position = self.limits.limit(position)
        steps = self.position_to_steps_conversion(desired_position) - self.current_position

        # Check if the data is greater than the max value
        # If it is, set the data to the max value
        if steps > self.max_steps_can:
            steps = self.max_steps_can
        elif steps < -self.max_steps_can:
            steps = -self.max_steps_can

        steps *= self.max_steps_percent

        return steps

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

        # Limit desired position
        data = self.position_to_steps_conversion(
            self.limits.limit(self.current_position + self.steps_to_position_conversion(data))
        )

        return data

    def construct_frame(self, steps_to_move) -> jcan.Frame:
        """ Construct the jcan Frame based on how many steps to move """
        if not -self.max_steps_can <= steps_to_move <= self.max_steps_percent:
            self.logger.error(f"{steps_to_move} is outside of range [{-self.max_steps_can}, {self.max_steps_percent}]")
            return jcan.Frame(id=self.can_id, data=[0])

        # Pack the data into a list
        packed_data = list(pack('>b', int(steps_to_move * self.reversed))) # >b = big-endian signed byte (2 hex digits)

        # Create and return the frame
        frame = jcan.Frame(id=self.can_id, data=packed_data)

        return frame
