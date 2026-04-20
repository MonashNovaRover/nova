#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the URC UV Vis Spec Carousel

Is position controlled.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: CarouselController
TOPICS:
  - publisher: /science/carousel/feedback   [CarouselFeedback]
SERVICES:
  - service: /science/carousel/set_position [SetPosition]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - carousel/position         (in degrees)
STATE INTERFACES:
  - carousel/position         (in degrees)
  - <name>/sensor_load        (load feedback)
  - <name>/sensor_current     (current feedback)
  - <name>/zeroing            (zeroing in progress)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       12/04/26
EDITED:         20/04/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface
from rclpy.qos import QoSProfile, QoSHistoryPolicy, QoSDurabilityPolicy
from science_interfaces.msg import CarouselFeedback
from science_interfaces.srv import SetPosition
from science.urc.carousel_hardware import CarouselHardware


class CarouselController(Controller):
    # Command interfaces
    position_cmd: Interface

    # State interfaces
    position_state: Interface
    load_state: Interface
    current_state: Interface
    zeroing_state: Interface

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)

        # Set up params
        self.target_position = self.declare_parameter("initial_position", 0.0, "Initial position in degrees").value
        self.hardware_name = self.declare_parameter("hardware_name", "carousel", "Name of the carousel hardware").value

        # Track last published message for change detection
        self.last_feedback: Optional[CarouselFeedback] = None

        # Set up publisher with transient local QoS (keeps last message for new subscribers)
        qos_profile = QoSProfile(
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1,
            durability=QoSDurabilityPolicy.TRANSIENT_LOCAL
        )
        self.publisher = self.node.create_publisher(CarouselFeedback, "science/carousel/feedback", qos_profile)

        # Set up service
        self.set_position_service = self.node.create_service(SetPosition, "science/carousel/set_position", self.set_position_callback)

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        # Save references to interfaces
        self.position_cmd = command_interfaces["carousel/position"]
        self.position_state = state_interfaces["carousel/position"]
        self.load_state = state_interfaces[f"{self.hardware_name}/sensor_load"]
        self.current_state = state_interfaces[f"{self.hardware_name}/sensor_current"]
        self.zeroing_state = state_interfaces[f"{self.hardware_name}/zeroing"]

        return True

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        # Set target position
        self.position_cmd.value = self.target_position

        # Publish feedback if changed
        self.publish_if_changed()

    def publish_if_changed(self):
        """ Publish feedback message only if values have changed """
        msg = CarouselFeedback()
        msg.position = float(self.position_state.value)
        msg.load = float(self.load_state.value)
        msg.current = float(self.current_state.value)
        msg.zeroing = bool(self.zeroing_state.value)

        # Check if message has changed
        if self.last_feedback is None or not self._feedback_equal(msg, self.last_feedback):
            self.publisher.publish(msg)
            self.last_feedback = msg

    def _feedback_equal(self, a: CarouselFeedback, b: CarouselFeedback) -> bool:
        """ Compare two feedback messages for equality """
        return (
            a.position == b.position and
            a.load == b.load and
            a.current == b.current and
            a.zeroing == b.zeroing
        )

    def set_position_callback(self, request: SetPosition.Request, response: SetPosition.Response):
        """ Service callback to set the target position """
        self.target_position = request.position
        self.logger.info(f"Set carousel position to {request.position}°")
        response.success = True
        return response


if __name__ == "__main__":
    rclpy.init()
    node = Node("carousel")

    # URC 2026 Carousel system
    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller("controller", CarouselController, hardware_name="carousel") \
        .with_hardware("carousel", CarouselHardware) \
        .with_jcan() \
        .spin()
