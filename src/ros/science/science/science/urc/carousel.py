#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the URC UV Vis Spec Carousel

Is position controlled.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: CarouselController
TOPICS:
  - publisher: /science/<node_name>/feedback     [CarouselFeedback]
SERVICES:
  - service: /science/<node_name>/set_position   [SetPosition]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - carousel/position         (in degrees)
STATE INTERFACES:
  - carousel/position         (in degrees)
  - <name>/zeroing            (zeroing in progress)
  - <name>/is_moving          (carousel is moving)
  - hall_sensor/state         (hall effect sensor state)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       12/04/26
EDITED:         25/04/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface
from python_control2.hardware_interfaces import GenericSensorHardware
from rclpy.qos import QoSProfile, QoSHistoryPolicy, QoSDurabilityPolicy
from science_interfaces.msg import CarouselFeedback
from science_interfaces.srv import SetPosition
from science.urc.carousel_hardware import CarouselHardware
from science.urc.auger_hall_effect import create_hall_effect_interpreter


class CarouselController(Controller):
    # Command interfaces
    position_cmd: Interface

    # State interfaces
    position_state: Interface
    zeroing_state: Interface
    is_moving_state: Interface
    hall_state: Interface

    def __init__(self, contexts: Contexts, hardware_name: str = "carousel"):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param hardware_name: Name of the carousel hardware (default: "carousel")
        """
        super().__init__(contexts)

        # Set up params
        self.target_position = self.declare_parameter("initial_position", 0.0, "Initial position in degrees").value
        self.hardware_name = self.declare_parameter("hardware_name", hardware_name, "Name of the carousel hardware").value
        self.zero_target_on_zeroing = self.declare_parameter("zero_target_on_zeroing", True, "Set target position to 0 when zeroing").value

        # Track last published message for change detection
        self.last_feedback: Optional[CarouselFeedback] = None

        # Get node name for unique topic names (e.g., carousel_inner, carousel_outer)
        node_name = self.node.get_name()

        # Set up publisher with transient local QoS (keeps last message for new subscribers)
        qos_profile = QoSProfile(
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1,
            durability=QoSDurabilityPolicy.TRANSIENT_LOCAL
        )
        self.publisher = self.node.create_publisher(CarouselFeedback, f"science/{node_name}/feedback", qos_profile)

        # Set up service
        self.set_position_service = self.node.create_service(SetPosition, f"science/{node_name}/set_position", self.set_position_callback)

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
        self.zeroing_state = state_interfaces[f"{self.hardware_name}/zeroing"]
        self.is_moving_state = state_interfaces[f"{self.hardware_name}/is_moving"]
        self.hall_state = state_interfaces["hall_sensor/state"]

        return True

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        # Set target position
        self.position_cmd.value = self.target_position

        if self.zero_target_on_zeroing and self.zeroing_state.value and self.target_position != 0:
            self.target_position = 0
            self.logger.info(f"{self.node.get_name()} is zeroing, set target position to 0°")

    # Publish feedback if changed
        self.publish_if_changed()

    def publish_if_changed(self):
        """ Publish feedback message only if values have changed """
        msg = CarouselFeedback()
        msg.position = float(self.target_position)
        msg.zeroing = bool(self.zeroing_state.value)
        msg.hall_effect_triggered = bool(self.hall_state.value)
        msg.is_moving = bool(self.is_moving_state.value)

        # Check if message has changed
        if self.last_feedback is None or not self._feedback_equal(msg, self.last_feedback):
            self.publisher.publish(msg)
            self.last_feedback = msg

    def _feedback_equal(self, a: CarouselFeedback, b: CarouselFeedback) -> bool:
        """ Compare two feedback messages for equality """
        return (
            a.position == b.position and
            a.zeroing == b.zeroing and
            a.hall_effect_triggered == b.hall_effect_triggered and
            a.is_moving == b.is_moving
        )

    def set_position_callback(self, request: SetPosition.Request, response: SetPosition.Response):
        """ Service callback to set the target position """
        self.target_position = request.position
        self.logger.info(f"Set {self.node.get_name()} position to {request.position}°")
        response.success = True
        return response


if __name__ == "__main__":
    rclpy.init()
    node = Node("carousel")

    # Declare CAN data position parameter (0 = 0xX000, 1 = 0x0X00, 2 = 0x00X0, 3 = 0x000X, etc.)
    can_data_position = node.declare_parameter("hardware.hall_sensor.can_data_position", 1).value

    # URC 2026 Carousel system
    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller("controller", CarouselController, hardware_name="carousel") \
        .with_hardware("carousel", CarouselHardware) \
        .with_hardware("hall_sensor", GenericSensorHardware,
                      can_id=0x0E9,
                      interpret_data=create_hall_effect_interpreter(can_data_position),
                      unit="state",
                      initial_value=False) \
        .with_jcan() \
        .spin()
