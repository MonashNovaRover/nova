#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controls heaters using temperature sensors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TOPICS:
    - publisher: /science/thermal_data [ThermalData]
SERVICES:
	- service: /science/thermal_command [ThermalCommand]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - <heaters>/effort            [either 0 or 1]
STATE INTERFACES:
  - <temp_sensors>/temperature  [number in degrees Celsius]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Jonathan Jia, Binuda Kalugalage
CREATION:       09/01/2026
EDITED:         23/04/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from typing import Optional, Callable
from ..controller_manager.Interface import InterfaceCollection
from ..controller_manager.Contexts import Contexts
from ..controllers.Controller import Controller
from science_interfaces.srv import ThermalCommand
from science_interfaces.msg import ThermalData


class ThermalController(Controller):

    def __init__(self, contexts: Contexts,
                 temp_sensors: list[str] = None,
                 heaters: list[str] = None,
                 calculate_reference_temp: Callable[[list[float]], float] = lambda l: max(l),
                 command_service: str = "",
                 data_topic: str = "",
                 publish_rate: int = 5):
        """ Constructor for ThermalController

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param temp_sensors: List of temperature sensor names (as named in their hardware interfaces)
        :param heaters: List of heater names (uses "<heater_name>/effort" command interfaces)
        :param calculate_reference_temp: given list of temperatures from temp_sensors, return the
            "reference" temperature (determines if heater needs to be turned on or off)
        :param command_service: Name of service that changes heater settings (receives ThermalCommand)
        :param data_topic: Topic that ThermalData is published to regularly
        :param publish_rate: Frequency to which ThermalData is published to data_topic
        """
        super().__init__(contexts)

        # mutable default values
        if not temp_sensors:
            temp_sensors = [""]
        if not heaters:
            heaters = [""]

        self.temp_sensors: list[str] = self.declare_parameter("temp_sensors", temp_sensors).value
        self.heaters: list[str] = self.declare_parameter("heater", heaters).value
        self.calculate_reference_temp = calculate_reference_temp
        self.command_service: str = self.declare_parameter("command_service", command_service).value
        self.data_topic: str = self.declare_parameter("data_topic", data_topic).value
        self.publish_rate: int =  self.declare_parameter("publish_rate", publish_rate).value

        self.is_on = False
        self.target_temp = 0
        self.last_temperatures = [0] * len(self.temp_sensors)

        self.logger.info(f"ThermalController initialised with temp sensor names: {self.temp_sensors}"
                         f" and heater names: {self.heaters}")

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Gets state/command interfaces and creates publishers, service servers and timers

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        self.logger.debug(f"Getting {[h + "/effort" for h in self.heaters]} command interfaces")
        self.heater_cmds = [command_interfaces[h + "/effort"] for h in self.heaters]

        self.logger.debug("Getting temp_sensors/temperature state interface")
        self.temp_sensors_state = state_interfaces["temp_sensors/temperature"]

        self.thermal_data_publisher = self.node.create_publisher(ThermalData, self.data_topic, 5)
        self.publisher_timer = self.node.create_timer(1 / self.publish_rate, self.publish_thermal_data)

        self.thermal_command_service = self.node.create_service(ThermalCommand, self.command_service, self.thermal_command_callback)

        self.logger.debug(f"ThermalController {self.name} configured")

        return True

    def thermal_command_callback(self, request: ThermalCommand.Request, response: ThermalCommand.Response):
        """ Update heater control settings when received new ThermalCommand

        :param request: new settings for heater control (temperature, on or off)
        :param response: success or fail at applying new settings
        """
        self.is_on = request.state
        self.target_temp = request.target

        self.logger.info(f"ThermalController is now {"ON" if self.is_on else "OFF"} with a target temp of {self.target_temp}°C")

        response.success = True
        return response

    def publish_thermal_data(self):
        """ Publishes latest temperatures and status of thermal system """

        thermal_data_msg = ThermalData()
        thermal_data_msg.stamp = self.node.get_clock().now().to_msg()
        thermal_data_msg.state = self.is_on
        thermal_data_msg.temp = [round(t) for t in self.last_temperatures]
        self.thermal_data_publisher.publish(thermal_data_msg)

    def on_update(self, now: float, period: float):
        """ Update loop (called update_rate times per second)

        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        temperatures: list[float] = self.temp_sensors_state.value
        self.last_temperatures = temperatures

        reference_temp = self.calculate_reference_temp(temperatures)

        if self.is_on and reference_temp < self.target_temp:
            heater_effort = 1.0
        else:
            heater_effort = 0.0

        for heater_cmd in self.heater_cmds:
            heater_cmd.value = heater_effort
