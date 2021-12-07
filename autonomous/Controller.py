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
import sys

"""
TODO: publish drive commands
TODO: yaw_to method
TODO: update led according to distance?
TODO: go_to_waypoint
"""

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

        self.drive_cmd_publisher = self.create_publisher(DriveCmd, "auto_drive_commands", 10)
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
        Callback that appends the x-y position of a waypoint to the back of the waypoints list
        """
        self.waypoints.append([msg.data.x, msg.data.y])

    def __publish(self, drive_fraction, angular_fraction):
        """
        Publishes drive commands to the auto_drive_commands topic
        :param: drive_fraction: [-1:1]
        :param: angular_fraction: [-1:1]
        """

        # construct message to publish
        drive_cmd_msg = DriveCmd()

        drive_cmd_msg.speed = drive_fraction

        drive_cmd_msg.steer = angular_fraction

        # publish to public topic
        self.drive_cmd_publisher.publish(drive_cmd_msg)

    @staticmethod
    def print_update(action_msg, heading_to, yaw_diff, dist):
        pad = 10
        sys.stdout.write("\r" + "Action: " + action_msg.ljust(pad) + " | heading to: " + str(heading_to).ljust(pad)
                          + " | yaw diff: " + str(round(yaw_diff, 4)).ljust(pad) + " | distance: " + str(round(dist, 4)).ljust(pad))
        sys.stdout.flush()

    def __clear_waypoints(self):
        """
        empties the waypoints list - prevents further coordinates from being travelled to and allows path planning to be reset
        """
        self.waypoints = []

    def yaw_to(self, way_point):
        """
        TODO
        """
        pass

    def go_to_waypoint(self, waypoint, ask_for_input=False):
        pass

    def control(self):
        pass