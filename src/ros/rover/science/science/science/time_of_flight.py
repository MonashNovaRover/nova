#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TOF Controller to publish distance data to GUI
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: TimeOfFlightController
TOPICS:
  - publisher: /science/tof/distance    [sensor_msgs/msg/Range]
SERVICES:
    - /science/tof/reset                [std_srvs/srv/Trigger]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STATE INTERFACES:
  - tof_sensor/distance         [distance (mm)]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EVENTS:
  - reset/trigger
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        python_control2
AUTHOR(S):      Yahya Muayyiduddin, Felicity Matthews
CREATION:       24/12/2025
EDITED:         28/02/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection
from rclpy.qos import QoSProfile, QoSHistoryPolicy, QoSDurabilityPolicy
from sensor_msgs.msg import Range
from python_control2.hardware_interfaces import GenericSensorHardware, TriggerHardware
from std_srvs.srv import Trigger, Trigger_Request, Trigger_Response
from teleop_python_utils import EventCollection


class TimeOfFlightController(Controller):

    def __init__(self, contexts: Contexts, minimum_range: int, maximum_range: int):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"TimeOfFlightController Initialised")

        self.last_value = None

        # Do any setup logic here, save any contexts you want reference to in the future.
        # Keep last message in the topic for any new subscribers (can publish fewer messages)
        qos_profile = QoSProfile(
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1,
            durability=QoSDurabilityPolicy.TRANSIENT_LOCAL
        )
        self.tof_publisher = self.node.create_publisher(Range, "/science/tof/distance", qos_profile)
        self.minimum_range = self.declare_parameter("minimum_range", minimum_range).value
        self.maximum_range = self.declare_parameter("maximum_range", maximum_range).value

        # Setup reset service call
        self.reset_service = self.node.create_service(Trigger, "/science/tof/reset", self.reset_callback)

        # Get TOF reset event
        self.reset_event = None
        if EventCollection in contexts:
            events = contexts[EventCollection]
            self.reset_event = events.get(f"reset/trigger")
        else:
            self.logger.error("Could not find EventCollection in the python control contexts, cannot reset TOF sensor.")


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
        if self.distance.value != self.last_value:
            msg = Range()
            msg.range = float(self.distance.value)
            msg.min_range = float(self.minimum_range)
            msg.max_range = float(self.maximum_range)
            self.tof_publisher.publish(msg)
            self.last_value = self.distance.value


    def reset_callback(self, _: Trigger_Request, response: Trigger_Response):
        """ Reset Callback function when reset service is called """
        try:
            self.reset_event.invoke()
            self.logger.info("Successfully reset TOF sensor.")
            response.success = True

        except Exception as e:
            self.logger.error(f"An error occurred while attempting to reset the TOF sensor: {e}")
            response.success = False

        return response

if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("time_of_flight")
    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("TimeOfFlightController", TimeOfFlightController,
            minimum_range = 10,
            maximum_range = 100) \
        .with_hardware("reset", TriggerHardware,
            can_id=0x0E8,
            can_message=[]) \
        .with_hardware("tof_sensor", GenericSensorHardware,
            can_id = 0x4E1,
            interpret_data = lambda data: int(data[2]), # data in third byte
            unit = "distance") \
        .with_jcan() \
        .with_event_collection() \
        .spin()