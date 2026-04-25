#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controls heaters under the shot glasses
using temperature sensors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TOPICS:
    - publisher: /science/thermal_data [ThermalData]
SERVICES:
	- service: /science/thermal_command [ThermalCommand]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - <heaters>/effort            [either 0 or 1]
STATE INTERFACES:
  - <temp_sensors>/temperature  [number in degrees Celsius]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       17/04/2026
EDITED:         17/04/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from python_control2 import PythonControl
from python_control2.controllers import ThermalController
from python_control2.hardware_interfaces import QCMDHardware, GenericSensorHardware

if __name__ == "__main__":
    rclpy.init()

    node = Node("heater")
    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("controller", ThermalController,
                         temp_sensors = ["heater_sensor", "dirt_sensor"],
                         heaters = ["heater"],
                         calculate_reference_temp = lambda l: l[0], # use heater_sensor temperature as the current/reference temp
                         command_service = "/science/thermal_command",
                         data_topic = "/science/thermal_data") \
        .with_hardware("heater", QCMDHardware, can_id = 0x031) \
        .with_hardware("temp_sensors", GenericSensorHardware,
                       can_id=0x4E1,
                       interpret_data=lambda data: [((data[0] << 8) | data[1]) - 273.15,
                                                    ((data[2] << 8) | data[3]) - 273.15],
                       initial_value=[0.0, 0.0],
                       unit = "temperature") \
        .with_jcan() \
        .spin()