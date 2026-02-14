#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TOF Controller to publish distance data to GUI
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: TimeOfFlightController
TOPICS:
  - publisher: /science/analysis_arm Range

ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STATE INTERFACES:
  - tof_sensor/distance  [distance]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Yahya Muayyiduddin
CREATION:       24/12/2025
EDITED:         14/02/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
import jcan
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface
from sensor_msgs.msg import Range
from python_control2.hardware_interfaces import CMDHardware, GenericSensorHardware




class TimeOfFlightController(Controller):


    def __init__(self, contexts: Contexts, minimum_range: int, maximum_range: int):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"TimeOfFlightController Initialised")


        # Do any setup logic here, save any contexts you want reference to in the future.
        self.tof_publisher = self.create_publisher(Range, "/science/analysis_arm", 10)
        self.minimum_range = self.declare_parameter("minimum_range", minimum_range).value
        self.maximum_range = self.declare_parameter("maximum_range", maximum_range).value

        

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        self.distance = state_interfaces["tof_sensor/distance"]
        return True

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        msg = Range()
        msg.range = float(self.distance.value)
        msg.min_range = float(self.minimum_range)
        msg.max_range = float(self.maximum_range)
        self.tof_publisher.publish(msg)

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("time_of_flight")
    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("TimeOfFlightController", TsimeOfFlightController, 
            minimum_range = 10,
            maximum_range = 100) \
        .with_hardware("tof_sensor", GenericSensorHardware,
            can_id = 0x01, # Placeholder
            interpret_data = lambda data: int.from_bytes(data), # Placeholder
            unit = "distance") \
        .with_jcan() \
        .spin()