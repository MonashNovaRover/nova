#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Hydraprobe moisture probe sensor controller.
Receives humidity, temperature, conductivity, and salinity over CAN and publishes.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TOPICS:
    - publisher: /science/hydraprobe_data [HydraprobeData]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
STATE INTERFACES:
  - hydraprobe/humidity      [percent]
  - hydraprobe/temperature   [celsius]
  - hydraprobe/conductivity  [uS/cm]
  - hydraprobe/salinity      [ppt]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       25/04/2026
EDITED:         26/05/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from typing import Optional

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, QoSHistoryPolicy, QoSDurabilityPolicy
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
        self.last_conductivity = None
        self.last_salinity = None

    def on_configure(self, command_interfaces: InterfaceCollection,
                     state_interfaces: InterfaceCollection) -> Optional[bool]:
        """Configure controller by getting state interfaces and creating publisher."""
        self.humidity_state = state_interfaces["humidity/percent"]
        self.temperature_state = state_interfaces["temperature/celsius"]
        self.conductivity_state = state_interfaces["conductivity/uS/cm"]
        self.salinity_state = state_interfaces["salinity/ppt"]

        qos_profile = QoSProfile(
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1,
            durability=QoSDurabilityPolicy.TRANSIENT_LOCAL
        )
        self.publisher = self.node.create_publisher(HydraprobeData, self.data_topic, qos_profile)

        return True

    def on_update(self, now: float, period: float):
        """Read sensor values and publish only if changed."""
        humidity = float(self.humidity_state.value)
        temperature = float(self.temperature_state.value)
        conductivity = float(self.conductivity_state.value)
        salinity = float(self.salinity_state.value)

        if (humidity != self.last_humidity or temperature != self.last_temperature or
                conductivity != self.last_conductivity or salinity != self.last_salinity):
            self.last_humidity = humidity
            self.last_temperature = temperature
            self.last_conductivity = conductivity
            self.last_salinity = salinity

            msg = HydraprobeData()
            msg.moisture = humidity
            msg.temperature = temperature
            msg.conductivity = conductivity
            msg.dielectric = salinity  # Reusing dielectric field for salinity
            self.publisher.publish(msg)


if __name__ == "__main__":
    rclpy.init()

    node = Node("hydraprobe")
    PythonControl(node, update_rate=2, can_bus="can1") \
        .with_controller("controller", HydraprobeController) \
        .with_hardware("sensor", MultiSensorHardware,
                       can_id=0x4F6,
                       interpret_data_list=[
                           lambda x: float(int.from_bytes(x[0:2], 'big') / 100.0),  # temperature
                           lambda x: float(int.from_bytes(x[2:4], 'big') / 100.0),  # water content
                           lambda x: float(int.from_bytes(x[4:6], 'big') / 100.0),  # conductivity
                           lambda x: float(int.from_bytes(x[6:8], 'big') / 100.0),  # salinity
                       ],
                       hardware_names=["temperature", "humidity", "conductivity", "salinity"],
                       hardware_units=["celsius", "percent", "uS/cm", "ppt"],
                       initial_values=[0, 0, 0, 0]) \
        .with_jcan() \
        .spin()
