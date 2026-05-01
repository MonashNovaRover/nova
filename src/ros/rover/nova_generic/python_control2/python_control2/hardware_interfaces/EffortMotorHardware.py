"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Hardware interface for effort-based CAN motor control.
Unified interface supporting variable-length CAN
message formats for various motor types.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - <joint>/effort  [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Felicity Matthews, Brandon Chung
CREATION:       01/05/26
EDITED:         01/05/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import jcan

from ..controller_manager.Interface import Interface, InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts


class EffortMotorHardware(HardwareInterface):
    effort_cmd: Interface
    can_id: int
    # The name of the joint
    joint: str
    reversed: bool
    max_effort: float
    max_effort_can: int
    send_single_zero: bool
    packed_data_length: int
    _last_sent_data: int

    def __init__(self, contexts: Contexts,
                 joint: str="",
                 can_id: int=0,
                 reversed: bool=False,
                 max_effort: float=1.0,
                 max_effort_can: int=None,
                 packed_data_length: int=2,
                 send_single_zero: bool=True):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param joint: Name of the joint (defaults to hardware interface name)
        :param can_id: CAN ID of the motor
        :param reversed: Whether the output should be reversed
        :param max_effort: Max percentage of output to send (0.0 to 1.0)
        :param max_effort_can: Max CAN message value that can be sent (auto-set based on packed_data_length if None)
        :param packed_data_length: Number of bytes used to pack the CAN data value
        :param send_single_zero: Only send one zero command instead of spamming zeros
        """
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]

        # Default joint name to the hardware interface name
        if len(joint) == 0:
            joint = self.name

        # Auto-set max_effort_can based on packed_data_length if not provided
        if max_effort_can is None:
            max_effort_can = (1 << (packed_data_length * 8 - 1)) - 1

        self.declare_parameter("joint", joint, "Name of the joint")
        self.declare_parameter("can_id", can_id, "CAN ID of the motor")
        self.declare_parameter("reversed", reversed, "Whether the output should be reversed")
        self.declare_parameter("max_effort", max_effort, "Max percentage of output to send")
        self.declare_parameter("max_effort_can", max_effort_can, "Max CAN message value that can be sent")
        self.declare_parameter("packed_data_length", packed_data_length, "Number of bytes used to pack the CAN data value")
        self.declare_parameter("send_single_zero", send_single_zero, "Only send one zero command instead of spamming zeros")

        self._last_sent_data = None

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
        self.packed_data_length = self.get_parameter("packed_data_length").value
        self.send_single_zero = self.get_parameter("send_single_zero").value

        # Validate packed_data_length
        if self.packed_data_length <= 0:
            self.logger.error(f"EffortMotorHardware \"{self.name}\" has invalid packed_data_length: {self.packed_data_length}. Must be positive.")
            return False

        # Get command interfaces
        self.effort_cmd = command_interfaces[self.joint + "/effort"]

        # Validate command interface configuration
        if not self.effort_cmd:
            self.logger.warn(f"EffortMotorHardware \"{self.name}\" has no populated command interfaces. "
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
        frame = self.construct_frame()

        # If send_single_zero is enabled, check if we should skip sending
        if self.send_single_zero:
            # Extract the data value from the frame
            current_data = int.from_bytes(bytes(frame.data), byteorder='big', signed=True)

            # If current data is zero and last sent was also zero, skip sending
            if current_data == 0 and self._last_sent_data == 0:
                return

            # Update last sent data and send the frame
            self._last_sent_data = current_data

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

        # Pack the data into a list using to_bytes
        packed_data = list(data.to_bytes(self.packed_data_length, byteorder='big', signed=True))

        # Create and return the frame
        frame = jcan.Frame(id=self.can_id, data=packed_data)

        return frame


# ============================================================================
# Specific Board Wrapper Classes
# ============================================================================

class QCMDHardware(EffortMotorHardware):
    """
    Hardware interface for Quad CAN Motor Drivers (QCMD).
    This is a thin wrapper around EffortMotorHardware configured for 2-byte messages.
    """
    def __init__(self, contexts: Contexts, joint: str="", can_id: int=0, reversed: bool=False,
                 max_effort: float=1.0, max_effort_can: int=0x7FFF, send_single_zero: bool=True):
        super().__init__(contexts, joint=joint, can_id=can_id, reversed=reversed, max_effort=max_effort,
                        max_effort_can=max_effort_can, packed_data_length=2, send_single_zero=send_single_zero)


class ContinousServoHardware(EffortMotorHardware):
    """
    Hardware interface for continuous servo motors.
    This is a thin wrapper around EffortMotorHardware configured for 1-byte messages.
    """
    def __init__(self, contexts: Contexts, joint: str="", can_id: int=0, reversed: bool=False,
                 max_effort: float=1.0, max_effort_can: int=0x7F, send_single_zero: bool=True):
        super().__init__(contexts, joint=joint, can_id=can_id, reversed=reversed, max_effort=max_effort,
                        max_effort_can=max_effort_can, packed_data_length=1, send_single_zero=send_single_zero)
