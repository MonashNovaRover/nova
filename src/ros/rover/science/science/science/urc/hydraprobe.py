#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Hydraprobe moisture probe sensor controller.
Receives humidity and temperature over CAN and publishes.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TOPICS:
    - publisher: /science/hydraprobe_data [HydraprobeData]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STATE INTERFACES:
  - hydraprobe/humidity     [percentage]
  - hydraprobe/temperature  [degrees C]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       25/04/2026
EDITED:         25/04/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from typing import Optional

import rclpy
from rclpy.node import Node
from python_control2 import PythonControl, Controller
from python_control2.controller_manager.Interface import InterfaceCollection
from python_control2.controller_manager.Contexts import Contexts
from python_control2.hardware_interfaces import MultiSensorHardware
from science_interfaces.msg import HydraprobeData


class HydraprobeController(Controller):
    """Controller that publishes hydraprobe sensor data only when values change."""

    def __init__(self, contexts: Contexts,
                 data_topic: str = "/science/hydraprobe_data"):
        """
        :param contexts: Dependency injection contexts.
        :param data_topic: Topic to publish HydraprobeData to.
        """
        super().__init__(contexts)

        self.data_topic: str = self.declare_parameter("data_topic", data_topic).value

        self.last_humidity = None
        self.last_temperature = None

    def on_configure(self, command_interfaces: InterfaceCollection,
                     state_interfaces: InterfaceCollection) -> Optional[bool]:
        """Configure controller by getting state interfaces and creating publisher."""
        self.humidity_state = state_interfaces["humidity/percent"]
        self.temperature_state = state_interfaces["temperature/celsius"]

        self.publisher = self.node.create_publisher(HydraprobeData, self.data_topic, 10)

        return True

    def on_update(self, now: float, period: float):
        """Read sensor values and publish only if changed."""
        humidity = float(self.humidity_state.value)
        temperature = float(self.temperature_state.value)

        if humidity != self.last_humidity or temperature != self.last_temperature:
            self.last_humidity = humidity
            self.last_temperature = temperature

            msg = HydraprobeData()
            msg.moisture = humidity
            msg.temperature = temperature
            msg.conductivity = 0.0  # Not provided over CAN
            msg.dielectric = 0.0    # Not provided over CAN
            self.publisher.publish(msg)


if __name__ == "__main__":
    rclpy.init()

    node = Node("hydraprobe")
    PythonControl(node, update_rate=2, can_bus="can1") \
        .with_controller("controller", HydraprobeController) \
        .with_hardware("sensor", MultiSensorHardware,
                       can_id=0x4F6,
                       interpret_data_list=[
                           lambda data: data[0],  # Humidity is byte 0
                           lambda data: data[1],  # Temperature is byte 1
                       ],
                       hardware_names=["humidity", "temperature"],
                       hardware_units=["percent", "celsius"],
                       initial_values=[0, 0]) \
        .with_jcan() \
        .spin()
