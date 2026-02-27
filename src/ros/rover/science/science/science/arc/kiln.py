#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controls heaters in the kiln using temperature sensors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: kiln
TOPICS:
    - publisher: /science/kiln_data [KilnData]
SERVICES:
	- service: /science/kiln_command [KilnCommand]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - <heaters>/effort            [either 0 or 1]
STATE INTERFACES:
  - <temp_sensors>/temperature  [number in degrees Celsius]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Jonathan Jia
CREATION:       09/01/2026
EDITED:         25/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional, Callable
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface
from python_control2.hardware_interfaces import QCMDHardware, GenericSensorHardware
from science_interfaces.srv import KilnCommand
from science_interfaces.msg import KilnData


class HeaterController(Controller):

    def __init__(self, contexts: Contexts,
                 temp_sensors: list[str] = None,
                 heaters: list[str] = None,
                 calculate_reference_temp: Callable[[list[float]], float] = lambda l: max(l),
                 command_service: str = "",
                 data_topic: str = "",
                 publish_rate: int = 5):
        """ Constructor for HeaterController

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param temp_sensors: List of temperature sensor names (as named in their hardware interfaces)
        :param heaters: List of heater names (uses "<heater_name>/effort" command interfaces)
        :param calculate_reference_temp: given list of temperatures from temp_sensors, return the
            "reference" temperature (determines if heater needs to be turned on or off)
        :param command_service: Name of service that changes heater settings (receives KilnCommand)
        :param data_topic: Topic that KilnData is published to regularly
        :param publish_rate: Frequency to which KilnData is published to data_topic
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

        self.logger.info(f"HeaterController {self.name} initialised with temp sensor names: {self.temp_sensors}"
                         f" and heater names: {self.heaters}")

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Gets state/command interfaces and creates publishers, service servers and timers

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        self.logger.info(f"Getting {[h + "/effort" for h in self.heaters]} command interfaces")
        self.heater_cmds = [command_interfaces[h + "/effort"] for h in self.heaters]

        self.logger.info(f"Getting {[t + "/temperature" for t in self.temp_sensors]} state interfaces")
        self.temp_sensor_states = [state_interfaces[t + "/temperature"] for t in self.temp_sensors]

        self.kiln_data_publisher = self.node.create_publisher(KilnData, self.data_topic, 5)
        self.publisher_timer = self.node.create_timer(1 / self.publish_rate, self.publish_kiln_data)

        self.kiln_command_service = self.node.create_service(KilnCommand, self.command_service, self.kiln_command_callback)

        self.logger.info(f"HeaterController {self.name} configured")

        return True

    def kiln_command_callback(self, request: KilnCommand.Request, response: KilnCommand.Response):
        """ Update heater control settings when received new KilnCommand

        :param request: new settings for heater control (temperature, on or off)
        :param response: success or fail at applying new settings
        """
        self.logger.info(f"HeaterController {self.name} received KilnCommand request: {request}")

        self.is_on = request.state
        self.target_temp = request.target

        response.success = True
        return response

    def publish_kiln_data(self):
        """ Publishes latest temperatures and status of kiln """

        kiln_data_msg = KilnData()
        kiln_data_msg.state = self.is_on
        kiln_data_msg.temp = [round(t) for t in self.last_temperatures]
        self.kiln_data_publisher.publish(kiln_data_msg)

        self.logger.debug(f"HeaterController {self.name} published KilnData: {kiln_data_msg}")

    def on_update(self, now: float, period: float):
        """ Update loop (called update_rate times per second)

        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        temperatures: list[float] = list(map(lambda s: s.value, self.temp_sensor_states))
        self.last_temperatures = temperatures

        reference_temp = self.calculate_reference_temp(temperatures)

        if self.is_on and reference_temp < self.target_temp:
            heater_effort = 1.0
        else:
            heater_effort = 0.0
        
        for heater_cmd in self.heater_cmds:
            heater_cmd.value = heater_effort

        self.logger.debug(f"HeaterController {self.name} updated: "
                          f"{temperatures} temperatures -> {heater_effort} heater effort")

if __name__ == "__main__":
    rclpy.init()

    node = Node("kiln")
    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("controller", HeaterController,
                         temp_sensors = ["kiln_sensor", "condenser_sensor"],
                         heaters = ["left_heater", "right_heater"],
                         calculate_reference_temp = lambda l: l[0], # use kiln_sensor temperature as the current/reference temp
                         command_service = "/science/kiln_command",
                         data_topic = "/science/kiln_data") \
        .with_hardware("left_heater", QCMDHardware, can_id = 0x41) \
        .with_hardware("right_heater", QCMDHardware, can_id = 0x42) \
        .with_hardware("kiln_sensor", GenericSensorHardware,
                       can_id = 0x02,
                       interpret_data = lambda data: 0.02 * int.from_bytes(data) - 273.15,
                       unit = "temperature") \
        .with_hardware("condenser_sensor", GenericSensorHardware,
                       can_id = 0x03,
                       interpret_data = lambda data: 0.02 * int.from_bytes(data) - 273.15,
                       unit = "temperature") \
        .with_jcan() \
        .spin()