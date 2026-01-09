#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
<insert purpose here>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: TBD
TOPICS:
  - publisher: <topic> [<msg type>]
SERVICES:
	- service: <service> [<srv type>]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Jonathan Jia
CREATION:       09/01/2026
EDITED:         09/01/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional, Callable
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface
from python_control2.hardware_interfaces import CMDHardware, GenericSensorHardware
from science_interfaces.srv import KilnCommand
from science_interfaces.msg import KilnData


class HeaterController(Controller):

    def __init__(self, contexts: Contexts,
                 thermistors: list[str] = None,
                 cmd_name: str = "",
                 calculate_reference_temp: Callable[[list[float]], float] = lambda l: max(l),
                 command_service: str = "",
                 data_topic: str = "",
                 publish_rate: int = 5):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param thermistors: List of names given to each thermistor (as specified
        """
        super().__init__(contexts)

        if not thermistors:
            thermistors = [""] # mutable default value

        self.declare_parameter("thermistors", thermistors)
        self.declare_parameter("cmd_name", cmd_name)
        self.calculate_reference_temp = calculate_reference_temp
        self.declare_parameter("command_service", command_service)
        self.declare_parameter("data_topic", data_topic)
        self.declare_parameter("publish_rate", publish_rate)

        self.is_on = False
        self.target_temp = 0
        self.last_temperatures = [0] * len(thermistors)

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """

        self.thermistors: list[str] = self.get_parameter("thermistors").value
        self.cmd_joint: str = self.get_parameter("cmd_name").value
        self.command_service: str = self.get_parameter("command_service").value
        self.data_topic: str = self.get_parameter("data_topic").value
        self.publish_rate: int = self.get_parameter("publish_rate").value

        self.heater_cmd = command_interfaces[self.cmd_joint + "/effort"]
        self.logger.debug(f"HeaterController {self.name} populated command interface {self.cmd_joint}/effort")

        def get_state_interface(thermistor: str):
            new_interface: Interface[float] = state_interfaces[thermistor + "/value"]
            if new_interface:
                self.logger.debug(f"HeaterController {self.name} found populated state interface {thermistor}/value")
            else:
                self.logger.warn(f"HeaterController {self.name} found state interface {thermistor}/value unpopulated")
            return new_interface

        self.thermistor_states = list(map(get_state_interface, self.thermistors))

        self.kiln_data_publisher = self.node.create_publisher(KilnData, self.data_topic, 5)
        self.publisher_timer = self.node.create_timer(1 / self.publish_rate, self.publish_kiln_data)

        self.kiln_command_service = self.node.create_service(KilnCommand, self.command_service, self.on_kiln_command)

        self.logger.info(f"HeaterController {self.name} started")

        return True

    def on_kiln_command(self, request: KilnCommand.Request, response: KilnCommand.Response):
        self.logger.debug(f"HeaterController {self.name} received KilnCommand request: {request}")

        self.is_on = request.state
        self.target_temp = request.target

        response.success = True
        return response

    def publish_kiln_data(self):
        kiln_data_msg = KilnData()
        kiln_data_msg.state = self.is_on
        kiln_data_msg.temp = self.last_temperatures
        self.kiln_data_publisher.publish(kiln_data_msg)

        self.logger.debug(f"HeaterController {self.name} published KilnData: {kiln_data_msg}")

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        temperatures: list[float] = list(map(lambda s: s.value, self.thermistor_states))
        self.last_temperatures = temperatures

        reference_temp = self.calculate_reference_temp(temperatures)

        if self.is_on and reference_temp < self.target_temp:
            self.heater_cmd.value = 1.0
        else:
            self.heater_cmd.value = 0.0

        self.logger.debug(f"HeaterController {self.name} updated: "
                          f"{temperatures} temperatures -> {self.heater_cmd.value} heater level")

if __name__ == "__main__":
    rclpy.init()

    # example node (to be replaced)
    node = Node("heater_controller_example")
    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("kiln_controller", HeaterController,
                         thermistors = ["kiln_thermistor", "condenser_thermistor"],
                         cmd_name = "kiln_heater",
                         calculate_reference_temp = lambda l: l[0], # use kiln_thermistor temperature as the current/reference temp
                         command_service = "/science/kiln_command",
                         data_topic = "/science/kiln_data") \
        .with_hardware("kiln_heater", CMDHardware, can_id = 0x051) \
        .with_hardware("kiln_thermistor", GenericSensorHardware,
                       can_id = 0x4B1,
                       interpret_data = lambda data: -0.0169 * int.from_bytes(data) + 80.797) \
        .with_hardware("condenser_thermistor", GenericSensorHardware,
                       can_id = 0x4A1,
                       interpret_data = lambda data: -0.0169 * int.from_bytes(data) + 80.797) \
        .with_jcan() \
        .spin()