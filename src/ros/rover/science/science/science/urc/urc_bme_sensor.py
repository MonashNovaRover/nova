#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the URC BME Sensors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: URCBMESensorController
TOPICS:
  - publisher: /science/bme_sensor [BMESensor]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STATE INTERFACES:
  - [<sensors>/<unit>]          [list of bme sensor readings]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Yahya Muayyiduddin
CREATION:       27/02/2026 
EDITED:         21/03/2026 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
import jcan
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface
from science_interfaces.msg import BMESensor
from python_control2.hardware_interfaces import CMDHardware, MultiSensorHardware

CAN_BUS = "can1"



BME_TEMP_NAME = "bme_temperature"
BME_PRESSURE_NAME = "bme_pressure"
BME_HUMIDITY_NAME = "bme_humidity"


class URCBMESensorController(Controller):
    # Command interfaces
    # joint_cmd: Interface

    # State interfaces
    # state: Interface

    def __init__(self, contexts: Contexts, sensors: list[str], units: list[str], data_topic: str):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"URCBMESensorController -- I have been __init__ialized")
           
        if len(sensors) == len(units):
            self.sensors_names = self.declare_parameter("sensor_names", sensors).value
            self.units = self.declare_parameter("sensor_units", units).value
            self.sensors = list(zip(sensors, units))
        else:
            self.logger.error(f"sensors array and units array are not the same size")
        self.sensor_last_readings = {
            f"{name}/{unit}": 0.0
            for name, unit in self.sensors
        }    
        self.data_topic = self.declare_parameter("data_topic", data_topic).value
        self.bme_publisher = self.node.create_publisher(BMESensor, self.data_topic, 10)
    
        

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection, ) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        # Save references to interfaces

        self.sensor_states = {}
        for sensor in self.sensors:
            sensor_name, sensor_unit = sensor
            self.sensor_states[f"{sensor_name}/{sensor_unit}"] = state_interfaces[f"{sensor_name}/{sensor_unit}"]
        self.publisher_timer = self.node.create_timer(1 / 10, self.publish_data)
        return True


    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        # Update Command Interfaces

        for sensor_name, sensor_unit in self.sensors:
            self.sensor_last_readings[f"{sensor_name}/{sensor_unit}"] = self.sensor_states[f"{sensor_name}/{sensor_unit}"].value
            


    def publish_data(self):
        msg = BMESensor()
        msg.temperature = float(self.sensor_last_readings[f"{BME_TEMP_NAME}/temperature"])
        msg.humidity = float(self.sensor_last_readings[f"{BME_HUMIDITY_NAME}/humidity"])
        msg.pressure = float(self.sensor_last_readings[f"{BME_PRESSURE_NAME}/pressure"])
        self.bme_publisher.publish(msg)

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    sensor_names = [BME_TEMP_NAME, BME_PRESSURE_NAME, BME_HUMIDITY_NAME]
    sensor_units = ["temperature", "pressure", "humidity"]

    node = Node("urc_bme_sensor")
    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("URCBMESensorController", URCBMESensorController,
                    sensors=sensor_names,
                    units=sensor_units,
                    data_topic="/science/bme_sensor"
        ) \
        .with_hardware("bme_multisensor", MultiSensorHardware,
                        can_id = 0x4F5,
                        interpret_data_list = [
                            lambda x: float(x[0]) + (float(x[1]) / 100.0), 
                            lambda x: float(x[2]) + (float(x[3]) / 100.0), 
                            lambda x: float(x[4])                          
                        ],
                        hardware_names = sensor_names,
                        hardware_units = sensor_units,
                        initial_values = [0.0, 0.0, 0.0]) \
        .with_jcan() \
        .spin()