"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Hardware interface for miscellaneous sensors.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STATE INTERFACES:
  - <name>/<sensor_output>  [arbitrary value from interpret_data]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Jonathan Jia
CREATION:       13/01/26
EDITED:         25/01/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from collections.abc import Callable
from typing import Any

import jcan
from ..controller_manager.Interface import Interface, InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts

class GenericSensorHardware(HardwareInterface):

    def __init__(self, contexts: Contexts,
                 can_id: int = 0,
                 interpret_data: Callable[[bytes], Any] = lambda x: int.from_bytes(x),
                 sensor_output: str = "value",
                 initial_value: Any = 0):
        """ Constructor for GenericSensorHardware
        Creates state interface "name/sensor_output"

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param can_id: CAN ID of messages from the sensor
        :param interpret_data: Translates raw CAN data into sensor outputs (e.g. velocity, temperature)
        :param sensor_output: What the sensor outputs (e.g. velocity, temperature)
        :param initial_value: Value output before receiving first CAN messages
        """
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]
        self.interpret_data = interpret_data

        self.can_id: int = self.declare_parameter("can_id", can_id).value
        self.sensor_output: str = self.declare_parameter("sensor_output", sensor_output).value
        self.last_value: Any = self.declare_parameter("initial_value", initial_value).value

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        """ Used to set up your HardwareInterface. Run once before any other class method.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """

        # Get state interface
        self.output_state: Interface[Any] = state_interfaces[f"{self.name}/{self.sensor_output}"]

        # Validate state interface configuration
        if self.output_state:
            self.logger.debug(f"GenericSensorHardware \"{self.name}\" found populated state interface: "
                             f"(\"{self.name}/{self.sensor_output}\")")
        else:
            self.logger.warn(f"GenericSensorHardware \"{self.name}\" found state interface "
                             f"\"{self.name}/{self.sensor_output}\" unpopulated")

        self.bus.add_callback(self.can_id, self.frame_callback)

        self.logger.info(f"GenericSensorHardware {self.name} configured")

        return True

    def frame_callback(self, frame: jcan.Frame):
        self.last_value = self.interpret_data(frame.data)
        self.logger.debug(f"GenericSensorHardware \"{self.name}\" received CAN message: {frame}")

    def on_read(self, now: float, period: float):
        """ Called to read values from hardware, and put them into stored state interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        self.output_state.value = self.last_value

    def on_write(self, now: float, period: float):
        """ Called to write to hardware using values stored in command interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass
