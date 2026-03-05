#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controls NIR Sensors to take readings and publish readings to GUI
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STATE INTERFACES:
  - <sensor>/data    [An array of NIR Sensor readings]  
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: NIRProbeController
TOPICS:
  - publisher: /science/nir_probe_data [NIRProbeData]
SERVICES:
	- service: /science/take_nir_probe_reading [TakeNIRProbeReading]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EVENTS:
  - <sensor>_take_reading/trigger 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Yahya Muayyiduddin
CREATION:       28/2/2026
EDITED:         3/3/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
import jcan
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface
from science_interfaces.msg import NIRProbeData
from science_interfaces.srv import TakeNIRProbeReading
from python_control2.hardware_interfaces import TriggerHardware, GenericSensorHardware
from teleop_python_utils import Inputs, EventCollection


class NIRProbeController(Controller):
 


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


        self.last_sensor_values: list[int] = [0] * len(photodiode_sensors)
        self.taking_reading_status : bool = False
        self.reading_taken : bool = False

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

        self.sensor_state = state_interfaces[f"NIR_Sensors/data"] 
        self.take_reading_command = self.node.create_service(TakeNIRProbeReading, self.command_service, self.take_reading_callback)
        self.nir_data_publisher = self.node.create_publisher(NIRProbeData, self.data_topic, 5)
        self.logger.info(f"{"NIR_Sensors/data" in state_interfaces}")
        self.logger.info(f"{self.name} controller configured")
        return True

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """

        read_values : list[int] = self.sensor_state.value
        if read_values != self.last_sensor_values:
            self.last_sensor_values = read_values
            self.taking_reading_status = False
            self.reading_taken = True
            self.publish_msg()

       

    def take_reading_callback(self, _ : TakeNIRProbeReading.Request, response: TakeNIRProbeReading.Response):
        """ Callback function when take_nir_probe_reading service is called """
        try:
            self.take_reading_event.invoke()
            self.logger.info("Taking NIR reading.")
            response.success = True
            self.taking_reading_status = True
            self.publish_msg()
        except Exception as e:
            self.logger.error(f"An error occurred while attempting to take NIR reading: {e}")
            response.success = False
        return response 
    
    def publish_msg(self):
        msg = NIRProbeData()
        msg.data = self.last_sensor_values
        msg.reading_taken = self.reading_taken
        msg.taking_reading_status = self.taking_reading_status
        self.nir_data_publisher.publish(msg)


def calculate_photodiodes(data: bytes) -> list[int, int]:
        PD1_reading = int.from_bytes(data[2:4], "big")
        PD2_reading = int.from_bytes(data[0:2], "big")
        return [PD1_reading, PD2_reading]

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
        .with_hardware("NIR_Sensors", GenericSensorHardware,
                        can_id = 0x4E2,
                        interpret_data = calculate_photodiodes,
                        unit = "data",
                        initial_value = [0,0]) \
        .with_hardware("NIR_Probe_take_reading", TriggerHardware,
                        can_id=0x0E9,
                        can_message=[]) \
        .with_hardware("")
        .with_jcan() \
        .with_event_collection() \
        .spin()
 
        
        