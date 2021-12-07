#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This script is the controller node for the rover 
which receives the destination and processed map, 
then publishing the drive command for movement.
Receives pose updates and waypoints via subscribers
and publishes drive commands. Converted to Ros2 by
Max Tory from initial code by Aidan Pritchard and 
Liam Whittle
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: Controller
TOPICS:
  - /D435/depth/color/points [sensor_msgs.msg.PointCloud2]
SERVICES:
  - None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        autonomous
AUTHOR(S):      Max Tory
CREATION:       07/12/2021
EDITED:         07/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
from controller_math import *
import controller_params
from core.msg import DriveCmd, RoverPose, Waypoint

class Controller(Node):
    """
    Controls the movement of the rover between waypoints determined by the path planner.
    Receives updates about the current pose of the rover and the waypoints to 
    navigate between via ros topics ------. Publishes drive commands to ---- 
    """
    def __init__(self):
        super.__init__('autonomous_controller_node')

        self.state = State()     # from controller_math
        self.waypoints = []
        self.max_distance = 0.0001      # furthest distance to an object? not sure

        self.drive_cmd_publisher = self.create_publisher(DriveCmd, "autonomous_drive_commands", 10)
        self.pose_subscriber = self.create_subscription(Pose, "auto_command_pose_updates", self.update_pose, 10)
        self.waypt_subscriber = self.create_subscription(Waypoint, "auto_command_waypoints", self.add_waypoint, 10)

    def update_pose(self, msg):
        """
        Callback function that updates the current pose of the rover from data in the auto_command_pose_updates topic
        """
        self.state.x = msg.data.x
        self.state.y = msg.data.y
        self.state.yaw = msg.data.yaw
        self.state.velocity = msg.data.velocity
        self.state.angular_velocity = msg.data.angular_velocity

    def add_waypoint(self, msg):
        """
        Appends the x-y position of a waypoint to the back of the waypoints list
        """
        self.waypoints.append([msg.data.x, msg.data.y])

    @staticmethod