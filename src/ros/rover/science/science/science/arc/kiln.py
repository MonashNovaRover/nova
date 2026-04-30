#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controls heaters in the kiln using temperature sensors
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: kiln
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
AUTHOR(S):      Jonathan Jia, Binuda Kalugalage
CREATION:       09/01/2026
EDITED:         23/04/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from python_control2 import PythonControl
from python_control2.controllers import ThermalController
from python_control2.hardware_interfaces import QCMDHardware, GenericSensorHardware


if __name__ == "__main__":
    rclpy.init()

    node = Node("kiln")
    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("controller", ThermalController,
                         temp_sensors = ["kiln_sensor", "condenser_sensor"],
                         heaters = ["left_heater", "right_heater"],
                         calculate_reference_temp = lambda l: l[0], # use kiln_sensor temperature as the current/reference temp
                         command_service = "/science/thermal_command",
                         data_topic = "/science/thermal_data") \
        .with_hardware("left_heater", QCMDHardware, can_id = 0x0C1) \
        .with_hardware("right_heater", QCMDHardware, can_id = 0x0D2) \
        .with_hardware("temp_sensors", GenericSensorHardware,
                       can_id=0x4E1,
                       interpret_data=lambda data: [((data[0] << 8) | data[1]) - 273.15,
                                                    ((data[2] << 8) | data[3]) - 273.15],
                       initial_value=[0.0, 0.0],
                       unit = "temperature") \
        .with_jcan() \
        .spin()
