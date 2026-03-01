#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
<insert purpose here>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STATE INTERFACES:
  - <sensor>/data    [data]  
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: NIRProbeController
TOPICS:
  - publisher: /science/nir_probe_data [NIRProbe]
SERVICES:
	- service: /science/take_nir_probe_reading [Service]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      <insert your name>
CREATION:       <current date>
EDITED:         <current date>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
import jcan
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface
from science_interfaces.msg import NIRProbe
from std_srvs.srv import Trigger, Trigger_Request, Trigger_Response
from python_control2.hardware_interfaces import CMDHardware
from teleop_python_utils import Inputs, EventCollection


class NIRProbeController(Controller):
    # Command interfaces
    # joint_cmd: Interface

    # State interfaces
    # state: Interface

    def __init__(self, contexts: Contexts, 
                hardware_name: str = "NIR_Probe",
                photodiode_sensors: list[str], 
                update_rate: int = 5,
                command_service: str,
                data_topic: str):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"NIRProbeController -- I have been __init__ialized")

        self.sensors: list[str] = self.declare_parameter("photodiode_sensors", photodiode_sensors).value
        self.hardware_name = self.declare_parameter("hardware_name", hardware_name).value
        self.data_topic = self.declare_parameter("data_topic", data_topic).value
        self.command_service = self.declare_parameter("command_service", command_service).value


        self.last_sensor_values = [0] * len(photodiode_sensors)


        self.take_reading_event = None
        if EventCollection in contexts:
            events = contexts[EventCollection]
            self.take_reading_event = events.get(f"{self.hardware_name}/take_reading")
        else:
            self.logger.error("Could not find EventCollection in the python control contexts, cannot take take NIR reading.")
        

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """

        self.sensor_states = [state_interfaces[f"{x}/data"] for x in self.sensors]
        self.take_reading_command = self.node.create_service(Trigger, self.command_service, self.take_reading_callback)
        self.nir_data_publisher = self.node.create_publisher(NIRProbe, self.data_topic, 5)

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """

        read_values = list(map(lambda x: x.value, self.sensor_states))
        if read_values != self.last_sensor_values:
            msg = NIRProbe()
            msg.data = [x for x in read_values]
            self.nir_data_publisher.publish(msg)
        self.last_sensor_values = read_values

    def take_reading_callback(self, _: Trigger_Request, response: Trigger_Response):
        """ Callback function when take_nir_probe_reading service is called """
        try:
            self.take_reading_event.invoke()
            self.logger.info("Taking NIR reading.")
            response.success = True

        except Exception as e:
            self.logger.error(f"An error occurred while attempting to take NIR reading: {e}")
            response.success = False

        return response 
    
    # def publish_reading(self):
    #     msg = NIRProbe()
    #     msg.data = [x for x in self.last_sensor_values]
    #     self.nir_data_publisher.publish(msg)


if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("control_test")
    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("NIRProbeController", NIRProbeController,
                        hardware_name = "NIR_Probe",
                        photodiode_sensors = ["PD1", "PD2"],
                        update_rate = 5,
                        command_service = "/science/take_nir_probe_reading",
                        data_topic = "/science/nir_probe_data") \
        .with_hardware("NIRProbeHardware", NIRProbeHardware,
                        hardware = "NIRProbeHardware",
                        can_id = 0x0E9) \
        .with_hardware("NIRProbeSensorHardware", NIRProbeSensorHardware,
                        sensors = ["PD1", "PD2"],
                        can_id = 0x4E2) \
        .with_jcan() \
        .with_event_collection() \
        .spin()