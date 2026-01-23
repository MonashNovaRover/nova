"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Hardware interface for positional servos.
Positional servos are used in the science payload.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - <servo>/position  [value between min_angle and 
                      max_angle]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Binuda Kalugalage
CREATION:       04/01/25
EDITED:         23/01/25
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import jcan

from ..controller_manager.Interface import Interface, InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts

class PositionalServoHardware(HardwareInterface):
    pos_cmd: Interface
    frame_id: int
    function_id: int

    def __init__(self, contexts: Contexts,
                 frame_id: int=0x0A0,
                 function_id: int=0x01,
                 angular_range: float=180.0,
                 min_angle: float=0.0,
                 max_angle: float=None, max_angle_can: int=0xFF):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]

        # Default max angle to the angular range
        if not max_angle:
            max_angle = angular_range

        self.declare_parameter("frame_id", frame_id, "Frame ID of the servo")
        self.declare_parameter("function_id", function_id, "Function ID of the servo")
        self.declare_parameter("angular_range", angular_range, "Angular range of the servo in degrees")
        self.declare_parameter("min_angle", min_angle, "Min allowable angle of the servo in degrees")
        self.declare_parameter("max_angle", max_angle, "Max allowable angle of the servo in degrees")
        self.declare_parameter("max_angle_can", max_angle_can, "Max CAN message value that can be sent")

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
        self.frame_id: int = self.get_parameter("frame_id").value
        self.function_id: int = self.get_parameter("function_id").value
        self.angular_range: float = self.get_parameter("angular_range").value

        self.min_angle = self.get_parameter("min_angle").value
        self.max_angle = self.get_parameter("max_angle").value
        self.max_angle_can = self.get_parameter("max_angle_can").value

        # Get command interfaces
        self.pos_cmd = command_interfaces[self.name + "/position"]

        # Validate command interface configuration
        if not self.pos_cmd:
            self.logger.warn(f'PositionalServoHardware "{self.name}" has no populated command interface '
                             f'("{self.name}/position")')

        # Validate angles
        if self.max_angle <= self.min_angle:
            self.logger.error(f'PositionalServoHardware {self.name} has invalid angle range ' 
                              f'min_angle={self.min_angle}, max_angle={self.max_angle}')
            return False

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
        if self.pos_cmd:
            self.bus.send(self.construct_frame())

    def construct_frame(self) -> jcan.Frame:
        """ Construct the jcan Frame based on current command interface """
        data = self.pos_cmd.value

        # Clamp to physical bounds
        if data > self.max_angle: 
            data = self.max_angle
        elif data < self.min_angle: 
            data = self.min_angle
        else:
            # Convert angle to CAN data using max CAN angle and angular range
            data = int(self.max_angle_can * data / self.angular_range)
      
        # Return the constructed frame
        return jcan.Frame(self.frame_id, [self.function_id, data])