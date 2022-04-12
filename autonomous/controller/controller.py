#!/usr/bin/env python3

__package__ = "autonomous"

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
        - Publishes: /autonomous/drive_inputs [DriveInput]
        - Subscribes: /rover/pose [RoverPose]
        - Subscribes: /autonomous/goals [Waypoints]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        autonomous
AUTHOR(S):      Max Tory
CREATION:       07/12/2021
EDITED:         07/12/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO: update led according to distance?
TODO: test all publishers and subscribers
TODO: investigate more efficient/accurate drive control methods than repeated tank turning and forward driving
"""

import rclpy
from rclpy.node import Node
from math_utils.controller_math import *
from config.runtime_params import *
from core.msg import DriveInput, RoverPose, Waypoints
from controller.YawStarController import YawStarController

from config.ros_config import *


class Controller(Node):
    """
    Controls the movement of the rover between waypoints determined by the path planner.
    Receives updates about the current pose of the rover and the waypoints to 
    navigate between via ros topics autonomous/pose and autonomous/goals. Publishes drive 
    commands to auto_drive_commands
    """
    CONTROLLER_MODE_DRIVE = YawStarController()
    CONTROLLER_MODE_SEARCH = 2  # at approximate position and searching for gate
    CONTROLLER_MODE_GATE = 3
    CONTROLLER_MODE_ACHIEVED = 4 # at waypoint and passed through gate

    def __init__(self):
        super().__init__('autonomous_controller_node')

        self.turning = YawStarController(self.get_logger())
        self.state = State()  # from controller_math
        self.waypoints = []
        self.target_waypoint = None
        self.previously_turned = False
        self.max_distance = 0.0001  # furthest distance to an object? not sure

        self.gate = None # URC gate to drive through

        self.controllers = {
            "drive" : YawStarController(),
            #"search" : SearchController(),
            #"gate" : GateController(),
        }

        self.active_controller = self.controllers["drive"]

        self.drive_cmd_publisher = self.create_publisher(DriveInput, auto_drive_command_topic, 10)
        self.pose_subscriber = self.create_subscription(RoverPose, rover_pose_topic, self.update_pose, 10)
        self.waypt_subscriber = self.create_subscription(Waypoints, auto_waypoints_topic, self.add_waypoints, 10)

        # Controls the rate at which drive commands are sent - sleeps for the necessary time to maintain the rate given
        self.timer = self.create_timer(0.1, self.control)

    def update_pose(self, msg):
        """
        Callback function that updates the current pose of the rover from data in the auto_command_pose_updates topic
        """
        self.state.x = msg.x
        self.state.y = msg.y
        self.state.yaw = msg.yaw
        self.state.velocity = msg.velocity
        self.state.angular_velocity = msg.angular_velocity

    def add_waypoints(self, msg):
        """
        Callback that appends the x-y position of a waypoint to the waypoints list
        """
        self.waypoints = [[point.x, point.y] for point in msg.waypoints]

    def __publish(self, drive_fraction, angular_fraction):
        """
        Publishes drive commands to the auto_drive_commands topic
        :param: drive_fraction: [-1:1]
        :param: angular_fraction: [-1:1]
        """

        # construct message to publish
        drive_cmd_msg = DriveInput()

        drive_cmd_msg.speed = drive_fraction

        drive_cmd_msg.steer = angular_fraction

        # publish to public topic
        self.drive_cmd_publisher.publish(drive_cmd_msg)

    def log_update(self, action_msg = '', heading_to = (0,0), yaw_diff = 0.0, dist = 0.0) -> None:
        """
        Logs the current action to ros logging
        """
        return
        pad = 10
        action = "Action: " + action_msg.ljust(pad) + " | heading to: " + str(heading_to).ljust(pad) + " | yaw diff: " \
                 + str(round(yaw_diff, 4)).ljust(pad) + " | distance: " + str(round(dist, 4)).ljust(pad)
        self.get_logger().info(action)

    def go_to_target(self):
        """
        Publishes a single drive command to navigate to the current target waypoint.
        Called every tick by the control method. Turns in place to face towards the waypoint, 
        or drives towards it in a straight line. If the rover has just finished turning, a
        single zero drive command is sent before driving begins.
        """
        try:
            drive = self.active_controller.get_drive_command(self.target_waypoint, self.state, self.gate)
            self.__publish(drive['drive'], drive['steer'])
        except Exception as e:
            self.get_logger().warn(str(e))

    def control(self):
        """
        Called once every tick by the node's timer. Identifies the next target waypoint
        and calls navigate_to_waypoint, and determines when the rover has arrived
        """

        if not self.target_waypoint:
            # There is currently no target - take the first waypoint on the list
            if self.waypoints:
                self.target_waypoint = self.waypoints.pop(0)
            else:
                return

        if distance((self.state.x, self.state.y), self.target_waypoint) > min_waypoint_distance:
            # we have not yet arrived at the waypoint
            self.go_to_target()

        else:
            # If distance to the waypoint is lower than the threshold distance, we have arrived
            self.get_logger().info("Reached way-point: " + str(self.target_waypoint))
            self.target_waypoint = None
            self.control()


def main(args=None):
    rclpy.init(args=args)
    controller = Controller()
    rclpy.spin(controller)
    controller.destroy_node()
    rclpy.shutdown()


def controller_test():
    rclpy.init(args=None)
    controller = Controller()
    rclpy.spin(controller)
    controller.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
    # controller_test()
