from collections.abc import Callable
from typing import Any

import jcan
from ..controller_manager.Interface import Interface, InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts

class GenericSensorHardware(HardwareInterface):

    def __init__(self, contexts: Contexts,
                 sensor_name: str = "",
                 can_id: int = 0,
                 interpret_data: Callable[bytes, Any] = lambda x: int.from_bytes(x)):
        """ Constructor, deferred until the control manager has been spun.

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param sensor_name: Name of the sensor
        :param can_id: CAN ID of messages from the sensor
        :param interpret_data: Translates CAN data into values output through the sensor_name/value state interface
        """
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]
        self.interpret_data = interpret_data
        self.last_value: Any = None

        # Default sensor name to the hardware interface name
        if len(sensor_name) == 0:
            sensor_name = self.name

        self.declare_parameter("sensor_name", sensor_name)
        self.declare_parameter("can_id", can_id)

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        """ Used to set up your HardwareInterface. Run once before any other class method.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        # Update params
        self.sensor_name: str = self.get_parameter("sensor_name").value
        self.can_id: int = self.get_parameter("can_id").value

        # Get state interface
        self.value_state: Interface[Any] = state_interfaces[self.sensor_name + "/value"]

        # Validate state interface configuration
        if not self.value_state:
            self.logger.warn(f"GenericSensorHardware \"{self.name}\" has no populated state interface. "
                             f"(\"{self.sensor_name}/value\")")

        self.bus.add_callback(self.can_id, self.frame_callback)

        return True

    def frame_callback(self, frame: jcan.Frame):
        self.last_value = self.interpret_data(frame.data)

    def on_read(self, now: float, period: float):
        """ Called to read values from hardware, and put them into stored state interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        self.value_state.value = self.last_value
        self.value_state.populated = True

    def on_write(self, now: float, period: float):
        """ Called to write to hardware using values stored in command interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass
