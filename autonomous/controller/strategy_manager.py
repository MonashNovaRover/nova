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
from core.msg import DriveInput, RoverPose, Waypoints, AutonomousGoal, AlvarMarker
from controller.drive_controller import DriveController
from controller.gate_controller import GateController
from controller.search_controller import SearchController

from config.ros_config import *


class StrategyManager(Node):
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
        super().__init__('autonomous_strategy_manager_node')
        self.mode = StrategyManager.DRIVE

        self.is_gate = False
        self.search = False
        self.tags = []
        self.ids = None
        self.found_ids = []

        self.goal = None

        self.state = None
        self.waypoints = []
        self.target_waypoint = None

        # Ros subscribers
        self.pose_subscriber = self.create_subscription(RoverPose, rover_pose_topic, self.update_pose, 10)
        self.ar_tag_subscriber = self.create_subscription(AlvarMarker, ar_track_topic, self.ar_goal_callback, 10)

        # ros publishers
        self.drive_cmd_publisher = self.create_publisher(DriveInput, auto_drive_command_topic, 10)
        self.goal_publisher = self.create_publisher(AutonomousGoal, planning_destination_topic, 10)

        # Controls the rate at which drive commands are sent - sleeps for the necessary time to maintain the rate given
        self.timer = self.create_timer(0.1, self.control)

    def update_pose(self, msg):
        """
        Callback function that updates the current pose of the rover from data in the auto_command_pose_updates topic
        """
        if self.state is None:
            self.state = State()
        self.state.x = msg.x
        self.state.y = msg.y
        self.state.yaw = msg.yaw
        self.state.velocity = msg.velocity
        self.state.angular_velocity = msg.angular_velocity

    def ar_goal_callback(self, msg):
        """
        1. Check that it's the AR tag we are looking for
        2. Filter out dodgy values
            - are values within an absolute range?
            - standard deviation? idk
        3. Transform pose of tag relative to rover into global pose of tag

        """
        if not self.search: return    # We aren't going to an AR tag
        if msg.id not in self.ids: return   # Not one of the ids we're looking for

        pose = msg.pose.pose.position

        local_pose = np.array([pose.x, pose.y])

        # tracking cam extrinsics are included in global pose as 0, 0 is the centre of the rover
        extrinsics = np.array(tracking_camera_extrinsics)[:2]
        local_pose -= extrinsics

        # distance from centre of rover to AR tag
        dist = (np.dot(local_pose, local_pose)) ** 0.5

        if not min_ar_distance <= dist <= max_ar_distance:
            return

        # translate step
        rot_mat = np.array(
            [[np.cos(self.state.yaw), -np.sin(self.state.yaw)], [np.sin(self.state.yaw), np.cos(self.state.yaw)]])
        local_pose.reshape(2, 1)

        global_pose = np.matmul(rot_mat, local_pose).reshape(2) + np.array([self.state.x, self.state.y])

        # Check the id of the tag against what we've found and either add or update it
        if msg.id in self.found_ids:
            # we've already found it, but we can update it
            self.tags[self.ids.index(msg.id)] = global_pose

        elif len(self.tags) == 0:
            self.found_ids.append(msg.id)
            self.tags.append(global_pose)

        elif len(self.tags) == 1 and self.is_gate:
            previous_tag = self.tags[0]
            if 3 > distance(np.array(previous_tag), np.array(global_pose)) > 1:
                self.found_ids.append(msg.id)
                self.tags.append(global_pose)

        self.goal = average_vector(self.tags)
        self.controllers[StrategyManager.SEARCH].new_goal(self.goal)

        self.get_logger().info("found tag: x=" + str(global_pose[0]) + " | y=" + str(global_pose[1]))
        self.get_logger().info(
            "Updated planning goal (AR tag): x=" + str(global_pose[0]) + "| y=" + str(global_pose[1])
        )

    def set_goal(self, msg):
        """
        Get the long-term goal for this autonomous cycle
        """
        self.send_autonomous_goal(msg.position)
        self.goal = (msg.position.x, msg.position.y)
        self.ids = [_id for _id in msg.ids]

        # set parameters of Search Controller
        self.controllers[StrategyManager.SEARCH].is_gate = msg.is_gate
        self.controllers[StrategyManager.SEARCH].search = msg.is_ar_tag

    def send_autonomous_goal(self, position):
        goal = AutonomousGoal()
        goal.position = position
        self.goal_publisher.publish(goal)

    def add_waypoints(self, msg):
        """
        Callback that appends the x-y position of a waypoint to the waypoints list
        """
        self.waypoints = [[point.x, point.y] for point in msg.path]

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
        pad = 10
        action = "Action: " + action_msg.ljust(pad) + " | heading to: " + str(heading_to).ljust(pad) + " | yaw diff: " \
                 + str(round(yaw_diff, 4)).ljust(pad) + " | distance: " + str(round(dist, 4)).ljust(pad)
        self.get_logger().debug(action)

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
            current_controller = StrategyManager.controllers[self.mode]

            drive = current_controller.get_drive_command(self.target_waypoint, self.state, self.goal, self.tags)
            self.__publish(drive['drive'], drive['steer'])

            if current_controller.completed():
                self.mode = self.next_mode[self.mode]

            if self.mode == StrategyManager.SUCCESS:
                self.achieved_goal()

        except Exception as e:
            self.get_logger().warn(str(e))

    def control(self):
        """
        Called once every tick by the node's timer. Identifies the next target waypoint
        and calls navigate_to_waypoint, and determines when the rover has arrived
        """
        if self.mode == StrategyManager.SUCCESS or self.state is None:
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
    controller = StrategyManager()
    rclpy.spin(controller)
    controller.destroy_node()
    rclpy.shutdown()


def controller_test():
    rclpy.init(args=None)
    controller = StrategyManager()
    rclpy.spin(controller)
    controller.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
    # controller_test()
