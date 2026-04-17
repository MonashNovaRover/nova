#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: connect to MAVLink UDP port opened by MAVProxy.
Use this connection to read location (lat, long, alt) from
the drone and publish this to ros2 topic /drone_gps/fix
so that the cartographer can read this and add the
drone to the GUI
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: drone_gps
TOPICS:
  - publisher: /drone_gps/fix [NavSatFix]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        drone
AUTHOR(S):      Henry Law, Victor Bartlinski
CREATION:       15/4/2026
EDITED:         15/4/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
from pymavlink import mavutil

class DroneGPS(Node):

    def __init__(self):
        super().__init__("DroneGPS")
        
        # define variables
        
        # add publishers
        
        # add services
        
        self.get_logger().info(f"{self.get_name()} started.")

def main():
    rclpy.init()
    node = DroneGPS()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()
