"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Hardware interface for sensors that need to send to multiple state interfaces
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STATE INTERFACES:
  - [<name>/<unit>] List of sensors and their units
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Yahya Muayyiduddin
CREATION:       21/03/26
EDITED:         26/03/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from collections.abc import Callable
from typing import Any

import jcan
from ..controller_manager.Interface import Interface, InterfaceCollection
from .HardwareInterface import HardwareInterface
from ..controller_manager.Contexts import Contexts

class MultiSensorHardware(HardwareInterface):

    def __init__(self, contexts: Contexts,
                 can_id: int = 0,
                 interpret_data_list: list[Callable[[bytes], Any]] = [lambda x: int.from_bytes(x)],
                 hardware_names: list[str] = ["hardware"],
                 hardware_units: list[str] = ["values"],
                 initial_values: list[Any] = [0]
                 ):
        """ 
        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param can_id: CAN ID of messages from the sensor
        :param interpret_data_list: List of functions, one per channel, that each extract a single value from the raw CAN frame bytes (e.g. parse bytes 0-1 as a scaled int for temp1).
        :param hardware_names: List of state interface names, one per channel (e.g. ["temp1", "temp2"]).
        :param hardware_units: List of units, one per channel (e.g. ["temperature", "temperature"]).
        :param initial_values: List of default values output before the first CAN message is received.
        """
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]
        self.interpret_data_list = interpret_data_list
        self.can_id: int = self.declare_parameter("can_id", can_id).value
        if len(hardware_names) == len(hardware_units) and len(hardware_names) == len(interpret_data_list):
            self.hardware_units: list[str] = self.declare_parameter("hardware_units", hardware_units).value
            self.hardware_names: list[str] = self.declare_parameter("hardware_names", hardware_names).value
        else:
            self.logger.error("Units, Names, and Interpret data arrays must be equal length")
        self.last_values: [Any] = self.declare_parameter("last_values", initial_values).value

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        """ Used to set up your HardwareInterface. Run once before any other class method.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """

        # Get state interface
        zipped_name_units :list[tuple[str, str]] = list(zip(self.hardware_names, self.hardware_units))
        self.state_interfaces : Interface[Any] = [state_interfaces[f"{hardware_name}/{hardware_unit}"] for hardware_name, hardware_unit in zipped_name_units]
        self.bus.add_callback(self.can_id, self.frame_callback)
        for name, unit in zipped_name_units:
            if state_interfaces[f"{name}/{unit}"]:
                 self.logger.debug(f"MultiSensorHardware \"{self.name}\" found populated state interface: "
                             f"(\"{name}/{unit}\")")
            else:
                self.logger.warn(f"MultiSensorHardware \"{self.name}\" found state interface "
                             f"\"{name}/{unit}\" unpopulated")

        self.logger.info(f"MultiSensorHardware {self.name} configured")

        return True

    def frame_callback(self, frame: jcan.Frame):
        self.last_values = [function(frame.data) for function in self.interpret_data_list]
        self.logger.debug(f"MultiSensorHardware \"{self.name}\" received CAN message: {frame}")

            
    

    def on_read(self, now: float, period: float):
        """ Called to read values from hardware, and put them into stored state interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        for i in range(len(self.state_interfaces)):
            self.state_interfaces[i].value = self.last_values[i]

    def on_write(self, now: float, period: float):
        """ Called to write to hardware using values stored in command interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass
