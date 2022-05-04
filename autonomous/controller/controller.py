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
from core.msg import DriveInput, RoverPose, Waypoints, AutonomousInfo, AutonomousGoal
from controller.drive_controller import DriveController
from controller.gate_controller import GateController
from controller.search_controller import SearchController
from nav_msgs.msg import Odometry

from config.ros_config import *


class Controller(Node):
    """
    Controls the movement of the rover between waypoints determined by the path planner.
    Receives updates about the current pose of the rover and the waypoints to 
    navigate between via ros topics autonomous/pose and autonomous/goals. Publishes drive 
    commands to auto_drive_commands
    """
    DRIVE = 0
    SEARCH = 1
    GATE = 2
    SUCCESS = 3

    controllers = {
        DRIVE: DriveController(),
        SEARCH: SearchController(),
        GATE: GateController(),
        SUCCESS: None
    }

    next_mode = {
        DRIVE: SEARCH,
        SEARCH: GATE,
        GATE: SUCCESS,
        SUCCESS: None
    }

    def __init__(self):
        super().__init__('autonomous_controller_node')
        self.mode = Controller.DRIVE

        self.is_gate = False
        self.search = False
        self.gate = None

        self.goal = None

        self.state = State()  # from controller_math
        self.waypoints = []
        self.target_waypoint = None

        # Ros subscribers and publishers
        self.drive_cmd_publisher = self.create_publisher(DriveInput, auto_drive_command_topic, 10)
        self.goal_publisher = self.create_publisher(AutonomousGoal, auto_goals_topic, 10)
        self.pose_subscriber = self.create_subscription(RoverPose, rover_pose_topic, self.update_pose, 10)
        self.waypt_subscriber = self.create_subscription(Waypoints, auto_waypoints_topic, self.add_waypoints, 10)
        self.ar_goal_subscriber = self.create_subscription(Odometry, ar_goals_topic, self.ar_goal_callback, 10)
        self.long_term_goal_subscriber = self.create_subscription(AutonomousInfo, auto_goals_info, self.set_goal, 10)

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

    def set_goal(self, msg):
        """
        Get the long-term goal for this autonomous cycle
        """
        goal = AutonomousGoal()
        goal.id = msg.id
        goal.position = msg.position
        self.goal_publisher.publish(goal)

        self.goal = (msg.position.x, msg.position.y)

        # set parameters of Search Controller
        self.controllers[Controller.SEARCH].is_gate = msg.is_gate
        self.controllers[Controller.SEARCH].search = msg.is_ar_tag

    def ar_goal_callback(self, msg):
        """
        1. Check that it's the AR tag we are looking for
        2. Filter out dodgy values
            - are values within an absolute range?
            - standard deviation? idk
        3. Transform pose of tag relative to rover into global pose of tag

        """
        self.controllers[Controller.SEARCH].new_goal(msg)
        self.goal = (msg.pose.pose.position.x, msg.pose.pose.position.y)

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

    def log_update(self, action_msg='', heading_to=(0, 0), yaw_diff=0.0, dist=0.0) -> None:
        """
        Logs the current action to ros logging
        """
        return
        pad = 10
        action = "Action: " + action_msg.ljust(pad) + " | heading to: " + str(heading_to).ljust(pad) + " | yaw diff: " \
                 + str(round(yaw_diff, 4)).ljust(pad) + " | distance: " + str(round(dist, 4)).ljust(pad)
        self.get_logger().info(action)

    def achieved_goal(self):
        """
        TODO: Set LED to flash with GOAL ACHIEVED
        """

    def go_to_target(self):
        """
        Publishes a single drive command to navigate to the current target waypoint.
        Called every tick by the control method. Turns in place to face towards the waypoint, 
        or drives towards it in a straight line. If the rover has just finished turning, a
        single zero drive command is sent before driving begins.
        """
        try:
            current_controller = Controller.controllers[self.mode]

            drive = current_controller.get_drive_command(self.target_waypoint, self.state, self.goal, self.gate)
            self.__publish(drive['drive'], drive['steer'])

            if current_controller.completed():
                self.gate = Controller.controllers[Controller.SEARCH].get_gate()
                self.mode = self.next_mode[self.mode]

            if self.mode == Controller.SUCCESS:
                self.achieved_goal()

        except Exception as e:
            self.get_logger().warn(str(e))

    def control(self):
        """
        Called once every tick by the node's timer. Identifies the next target waypoint
        and calls navigate_to_waypoint, and determines when the rover has arrived
        """
        if self.mode == Controller.SUCCESS:
            return

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
