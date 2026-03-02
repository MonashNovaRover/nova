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
from science_interfaces.msg import NIRProbeData
from std_srvs.srv import Trigger, Trigger_Request, Trigger_Response
from python_control2.hardware_interfaces import TriggerHardware, GenericSensorHardware
from teleop_python_utils import Inputs, EventCollection


class NIRProbeController(Controller):
    # Command interfaces
    # joint_cmd: Interface

    # State interfaces
    # state: Interface

    PHOTODIODES_OFF = 0
    PHOTODIODES_ON = 1


    def __init__(self, contexts: Contexts, 
                hardware_name: str = "NIR_Probe",
                photodiode_sensors: list[str] = [""], 
                update_rate: int = 5,
                command_service: str = "",
                data_topic: str = ""):
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
        self.status = 0
        self.reading_taken = False

        self.last_read_timer = self.node.create_timer(0.1, self.publish_msg)

        self.take_reading_event = None
        if EventCollection in contexts:
            events = contexts[EventCollection]
            self.take_reading_event = events.get(f"{self.hardware_name}_take_reading/trigger")
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
        self.nir_data_publisher = self.node.create_publisher(NIRProbeData, self.data_topic, 5)

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """

        read_values = list(map(lambda x: x.value, self.sensor_states))
        if read_values != self.last_sensor_values:
            self.last_sensor_values = read_values
            self.status = self.PHOTODIODES_OFF
            self.reading_taken = True
            self.publish_msg()

    def take_reading_callback(self, _: Trigger_Request, response: Trigger_Response):
        """ Callback function when take_nir_probe_reading service is called """
        try:

            self.take_reading_event.invoke()
            self.logger.info("Taking NIR reading.")
            response.success = True
            self.status = self.PHOTODIODES_ON
        except Exception as e:
            self.logger.error(f"An error occurred while attempting to take NIR reading: {e}")
            response.success = False

        return response 
    
    def publish_msg(self):
        msg = NIRProbeData()
        msg.data = [x for x in self.last_sensor_values]
        msg.reading_taken = self.reading_taken
        msg.status = self.status
        self.nir_data_publisher.publish(msg)

    @staticmethod
    def calculate_PD1(data: bytes) -> int:
        if len(data) < 8:
            raise ValueError(f"Expected 8 bytes, got {len(data)}")
        PD1_LEDon = int.from_bytes(data[2:4], "little")   # bytes 2-3
        PD1_LEDoff = int.from_bytes(data[6:8], "little")  # bytes 6-7
        PD1_diff = max(PD1_LEDon - PD1_LEDoff, 0)
   


        return PD1_diff

    @staticmethod
    def calculate_PD2(data: bytes) -> int:
        if len(data) < 8:
            raise ValueError(f"Expected 8 bytes, got {len(data)}")
        PD2_LEDon = int.from_bytes(data[0:2], "little")   # bytes 0-1
        PD2_LEDoff = int.from_bytes(data[4:6], "little")  # bytes 4-5
        PD2_diff = max(PD2_LEDon - PD2_LEDoff, 0)

        return PD2_diff


if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("nir_probe")
    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("NIRProbeController", NIRProbeController,
                        hardware_name = "NIR_Probe",
                        photodiode_sensors = ["PD1", "PD2"],
                        update_rate = 5,
                        command_service = "/science/take_nir_probe_reading",
                        data_topic = "/science/nir_probe_data") \
        .with_hardware("PD1", GenericSensorHardware,
                        can_id = 0x4E2,
                        interpret_data = lambda x: NIRProbeController.calculate_PD1(x),
                        unit = "data") \
        .with_hardware("PD2", GenericSensorHardware,
                        can_id = 0x4E2,
                        interpret_data = lambda x: NIRProbeController.calculate_PD2(x),
                        unit = "data") \
        .with_hardware("NIR_Probe_take_reading", TriggerHardware,
                        can_id=0x0E9,
                        can_message=[]) \
        .with_jcan() \
        .with_event_collection() \
        .spin()


        
        