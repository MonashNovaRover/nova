from typing import List

import jcan

from ..controller_manager.Interface import Interface, InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts
from enum import Enum
from struct import pack


class CanIdPrefix(Enum):
    SEND    = 0x0
    RECEIVE = 0x4

class CMDHardwareCommand(Enum):
    # Turns off the all the motor outputs.
    STOP                = 0x0
    # Powers the motor forward at roughly 90% power. Used for easy debugging
    TWITCH_FORWARD      = 0x1
    # Powers the motor in reverse at roughly 90% power. Used for easy debugging
    TWITCH_BACKWARDS    = 0x2
    # Drives the motor in open loop PWM mode. Takes in single signed integer. Sign dictates direction, magnitude
    # dictates duty cycle with the maximum value of 32767 being full power forward and the minimum of -32768 being full
    # power reverse.
    # Send with int16 data.
    PWM_DRIVE           = 0x3
    # Drives the motor in closed loop velocity control mode. Takes in single signed integer. Sign dictates direction,
    # magnitude dictates velocity target with the maximum value of 32767 being full speed forward and the minimum of
    # -32768 being full speed reverse. For specific motors there is a max velocity target recommended is between 70%
    # and 90% to allow it to be achieved by the cmds without clipping.
    # Send with int16 data.
    PID_DRIVE           = 0x4
    # Complicated. Sets the pid constants. Only used for tuning.
    PID_TUNE            = 0x5

class CMDHardwareHandle:
    def __init__(self, max_value: float, max_value_can: int, command: int):
        self.max_value = max_value
        self.max_value_can = max_value_can
        self.command = command

    def send_value(self, bus: jcan.Bus, can_id: int, value: float):
        frame_id : int = (CanIdPrefix.SEND.value << 8) | (can_id << 4) | self.command
        data = self.convert_to_can(value)
        bus.send(jcan.Frame(frame_id, data))

    def convert_to_can(self, value: float) -> List[int]:
        data: int = 0

        if value > self.max_value:
            data = self.max_value_can
        elif value < -self.max_value:
            data = -self.max_value_can
        else:
            data = int(self.max_value_can * value / self.max_value)

        return list(pack('>h', data))

class CMDHardware(HardwareInterface):
    effort_cmd: Interface
    velocity_cmd: Interface
    effort_handle: CMDHardwareHandle
    velocity_handle: CMDHardwareHandle
    can_id: int
    # The name of the joint
    joint: str

    def __init__(self, contexts: Contexts,
                 joint: str="",
                 can_id: int=0,
                 max_effort: float=1.0, max_effort_can: int=0x7FFF,
                 max_velocity: float=0.0, max_velocity_can: int=0x7FFF):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]

        # Default joint name to the hardware interface name
        if len(joint) == 0:
            joint = self.name

        self.declare_parameter("joint", joint)
        self.declare_parameter("can_id", can_id)
        self.declare_parameter("max_effort", max_effort)
        self.declare_parameter("max_effort_can", max_effort_can)
        self.declare_parameter("max_velocity", max_velocity)
        self.declare_parameter("max_velocity_can", max_velocity_can)

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

        # Validate the given can_id
        if self.can_id > 0x3F:
            self.logger.error(f"CMD CAN ID for {self.name} ({self.can_id}) is impossible. You should provide the CMD ID"
                              " part of the CAN frame ID, rather than the whole frame ID (omit the first and last hex "
                              "characters)\n\t0x043 -> bad\n\t0x4 -> good!")
            return False
        elif self.can_id > 0xF:
            self.logger.warn(f"CMD CAN ID for {self.name} ({self.can_id}) is unlikely to be correct. Make sure you only"
                             " provide the CMD ID part of the CAN frame ID, rather than the whole frame ID (omit the "
                             "first and last hex characters)\n\t0x043 -> bad\n\t0x4 -> good!")

        max_effort = self.get_parameter("max_effort").value
        max_effort_can = self.get_parameter("max_effort_can").value
        self.effort_handle = CMDHardwareHandle(max_effort, max_effort_can, CMDHardwareCommand.PWM_DRIVE.value)

        max_velocity = self.get_parameter("max_velocity").value
        max_velocity_can = self.get_parameter("max_velocity_can").value
        self.velocity_handle = CMDHardwareHandle(max_velocity, max_velocity_can, CMDHardwareCommand.PID_DRIVE.value)

        # Get command interfaces
        self.effort_cmd = command_interfaces[self.joint + "/effort"]
        self.velocity_cmd = command_interfaces[self.joint + "/velocity"]

        # Validate command interface configuration
        if self.effort_cmd and self.velocity_cmd:
            self.logger.error(f"You can only control either {self.joint}/effort or {self.joint}/velocity at any given "
                              f"time for CMDHardware \"{self.name}\", but not both!")
        elif not self.effort_cmd and not self.velocity_cmd:
            self.logger.warn(f"CMDHardware \"{self.name}\" has no populated command interfaces. "
                             f"(\"{self.joint}/effort\", \"{self.joint}/velocity\")")

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
        if self.effort_cmd:
            self.effort_handle.send_value(self.bus, self.can_id, self.effort_cmd.value)
        elif self.velocity_cmd:
            self.velocity_handle.send_value(self.bus, self.can_id, self.velocity_cmd.value)
        else:
            # Send a stop command
            frame_id : int = (CanIdPrefix.SEND.value << 8) | (self.can_id << 4) | CMDHardwareCommand.STOP.value
            self.bus.send(jcan.Frame(frame_id, []))
