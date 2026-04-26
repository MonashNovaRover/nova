#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the science Auger which actuates up
and down and drills, with hall effect sensor monitoring.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: auger_hall_effect
TOPICS:
  - publisher: /science/auger/hall_effect    [std_msgs/msg/Bool]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - actuation/effort    [value between -1 and 1]
  - drill/effort        [value between -1 and 1]
STATE INTERFACES:
  - hall_sensor/state    [boolean value]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       25/01/26
EDITED:         25/04/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface
from python_control2.hardware_interfaces import GenericSensorHardware, QCMDHardware
from rclpy.qos import QoSProfile, QoSHistoryPolicy, QoSDurabilityPolicy
from std_msgs.msg import Bool
from teleop_python_utils import Inputs
from science.auger import AugerController


class AugerHallEffectController(Controller):

    def __init__(self, contexts: Contexts, topic_name: str = "/science/auger/hall_effect"):
        """ Constructor for AugerHallEffectController

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param topic_name: Topic name to publish hall effect state (default: /science/auger/hall_effect)
        """
        super().__init__(contexts)

        # Declare topic parameter
        self.topic_name = self.declare_parameter("topic_name", topic_name, "Topic name for hall effect state").value

        # Track last published value to avoid redundant publishing
        self.last_value: Optional[bool] = None

        # Create publisher with transient local QoS (keeps last message for new subscribers)
        qos_profile = QoSProfile(
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1,
            durability=QoSDurabilityPolicy.TRANSIENT_LOCAL
        )
        self.publisher = self.node.create_publisher(Bool, self.topic_name, qos_profile)

        self.logger.info(f"AugerHallEffectController initialized, publishing to {self.topic_name}")

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Configure controller and get state interface

        :param command_interfaces: Collection of command interfaces (not used)
        :param state_interfaces: Collection of state interfaces
        :returns: True if configured successfully
        """
        # Get state interface from hall effect sensor hardware
        self.hall_state: Interface[bool] = state_interfaces["hall_sensor/state"]

        return True

    def on_update(self, now: float, period: float):
        """ Update loop - read sensor state and publish if changed

        :param now: Current time in seconds
        :param period: Time elapsed since last update in seconds
        """
        current_value = bool(self.hall_state.value)

        # Only publish if value has changed
        if self.last_value is None or current_value != self.last_value:
            msg = Bool()
            msg.data = current_value
            self.publisher.publish(msg)
            self.last_value = current_value
            self.logger.info(f"Hall effect sensor state changed to: {current_value}")


def create_hall_effect_interpreter(can_data_position: int):
    """
    Factory function to create a hall effect interpretation function.

    :param can_data_position: Which hex digit in CAN data to check:
                              0 = 0xX000 (first hex digit)
                              1 = 0x0X00 (second hex digit)
                              2 = 0x00X0 (third hex digit)
                              3 = 0x000X (fourth hex digit)
                              etc.
    :return: Interpretation function that maps CAN data to boolean
    """
    # Calculate which byte and which nibble
    byte_index = can_data_position // 2
    is_upper_nibble = (can_data_position % 2 == 0)

    def interpret_hall_effect(data: bytes) -> bool:
        if len(data) <= byte_index:
            return False

        byte_value = data[byte_index]

        if is_upper_nibble:
            # Check upper nibble (first hex digit): 0xX0
            return (byte_value & 0xF0) != 0
        else:
            # Check lower nibble (second hex digit): 0x0X
            return (byte_value & 0x0F) != 0

    return interpret_hall_effect


if __name__ == "__main__":
    rclpy.init()

    node = Node("auger_hall_effect")
    inputs = Inputs(node).with_topics("/science/input")

    # Declare CAN data position parameter (0 = 0xX000, 1 = 0x0X00, 2 = 0x00X0, 3 = 0x000X, etc.)
    can_data_position = node.declare_parameter("hardware.hall_sensor.can_data_position", 1).value

    # URC 2026 Auger system with Hall Effect monitoring
    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller("auger_controller", AugerController) \
        .with_controller("hall_effect_controller", AugerHallEffectController,
                        topic_name="/science/auger/hall_effect") \
        .with_hardware("actuation", QCMDHardware, can_id=0x031) \
        .with_hardware("drill", QCMDHardware, can_id=0x041) \
        .with_hardware("hall_sensor", GenericSensorHardware,
                      can_id=0x0E9,
                      interpret_data=create_hall_effect_interpreter(can_data_position),
                      unit="state",
                      initial_value=False) \
        .with_teleop(inputs) \
        .with_activation_buttons(
            start_active=True,
            active_button_name="activate_auger",
            inactive_button_pool_names=["activate_cbeam"]) \
        .with_jcan() \
        .spin()