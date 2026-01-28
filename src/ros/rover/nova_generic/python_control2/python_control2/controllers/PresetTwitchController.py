"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for systems which take joystick inputs 
to twitch and move to preset positions. 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - <hardware_name>/position   [value between 
                                min_angle and max_angle]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Binuda Kalugalage
CREATION:       24/01/2026
EDITED:         26/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from typing import Optional, Dict

from ..controller_manager.Interface import Interface, InterfaceCollection
from ..controller_manager.Contexts import Contexts
from ..controller_manager.Activation import Activation
from ..controllers.Controller import Controller
from teleop_python_utils import Inputs, Button


class PresetTwitchController(Controller):
    rotation_cmd: Interface

    def __init__(
        self,
        contexts: Contexts,
        min_angle: float = 0.0,
        max_angle: float = 180.0,
        positions: Dict[str, float] = None,
        twitch_max: float = 30.0,
        hardware_name: str = "rotation"):
        """ Constructor for PresetTwitchController

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param min_angle: Min allowable angle of the system, in degrees.
        :param max_angle: Max allowable angle of the system, in degrees.
        :param positions: The system's preset positions. 
        :param twitch_max: Maximum position the system will twitch, in degrees.
        :param hardware_name: Name of hardware interface being used.
        """

        super().__init__(contexts)
        self.logger.info(f"PresetTwitchController -- I have been __init__ialized")

        self.active = contexts[Activation]

        self.min_angle: float = self.declare_parameter("min_angle", min_angle).value
        self.max_angle: float = self.declare_parameter("max_angle", max_angle).value
        self.twitch_max: float = self.declare_parameter("twitch_max", twitch_max).value
        self.hardware_name: str = self.declare_parameter("hardware_name", hardware_name).value

        if positions is None:
            positions = {}

        # Get inputs
        inputs = contexts[Inputs]

        self.node_name = self.node.get_name()

        self.speed_axis_name = self.declare_parameter("speed_axis", f"{self.node_name}_speed").value
        self.button_twitch_increase_name = self.declare_parameter("twitch_increase_button", f"{self.node_name}_twitch_increase").value
        self.button_twitch_decrease_name = self.declare_parameter("twitch_decrease_button", f"{self.node_name}_twitch_decrease").value

        self.speed_axis = inputs.get_axis(self.speed_axis_name)
        self.button_twitch_increase = inputs.get_button(self.button_twitch_increase_name)
        self.button_twitch_decrease = inputs.get_button(self.button_twitch_decrease_name)

        # Dynamically create position and button params
        self.pose_positions: Dict[str, float] = {}
        self.pose_buttons: Dict[str, Button] = {}

        for pose, value in positions.items():
            self.pose_positions[pose] = self.declare_parameter(f"{pose}_pos", value).value
            self.pose_buttons[pose] = inputs.get_button(self.declare_parameter(f"{pose}_button", f"{self.node_name}_{pose}").value)

        # Set defaults
        if self.pose_positions:
            self.current_pos = self.pose_positions[next(iter(self.pose_positions))]
        else:
            self.current_pos = self.min_angle

        self.offset = 0.0

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        # Save references to interfaces
        self.logger.info(f"Getting {self.hardware_name}/position")
        self.rotation_cmd = command_interfaces[f"{self.hardware_name}/position"]

        # Validate angles
        if self.max_angle <= self.min_angle:
            self.logger.error(f'PresetTwitchController {self.node_name} has invalid angle range ' 
                              f'min_angle={self.min_angle}, max_angle={self.max_angle}')
            return False

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        if self.active is not None and not self.active:
            self.rotation_cmd.value = self.current_pos
            return

        # Update twitch amount
        twitch_step = self.get_speed() * self.twitch_max

        # Change to preset position
        for pose, button in self.pose_buttons.items():
            if button and self.current_pos != self.pose_positions[pose]:
                self.offset = 0.0
                self.current_pos = self.pose_positions[pose]
                self.logger.info(f"Moved to {pose.replace("_", " ").upper()} position: {self.current_pos}")
                break

        # Twitch/update offset
        if self.button_twitch_increase:
            self.twitch(twitch_step)
        elif self.button_twitch_decrease:
            self.twitch(-twitch_step)
        
        self.rotation_cmd.value = self.current_pos
    
    def twitch(self, step: float):
        """Updates the offset by applying a twitch step"""
        if step:
            updated_pos = self.current_pos + step

            # Clamp to physical bounds
            if updated_pos > self.max_angle:
                updated_pos = self.max_angle
            elif updated_pos < self.min_angle:
                updated_pos = self.min_angle

            # Only update if position has changed
            if updated_pos != self.current_pos:
                offset = updated_pos - self.current_pos
                self.current_pos = updated_pos
                self.logger.info(f"Moved to position: {self.current_pos} ({"+" if offset > 0 else ""}{offset})")

    def get_speed(self) -> float:
        """Gets the speed, mapping an axis [-1, 1] to a speed [0, 1]"""
        return (self.speed_axis.value + 1) / 2