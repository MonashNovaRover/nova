#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: TODO
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: deadzone_backout
TOPICS: TODO
SERVICES: TODO
ACTIONS: TODO
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Jonathan Jia
CREATION:	26/03/2026
EDITED:		TODO
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.time import Time
from rclpy.duration import Duration

# import custom messages
from drive_interfaces.msg import DriveInfo
from geometry_msgs.msg import TwistStamped

class DriveBackout(Node):

    def __init__(self):
        pass

if __name__ == "__main__":
    rclpy.init()
    monitor_node = DriveBackout()
    rclpy.spin(monitor_node)
    rclpy.shutdown()