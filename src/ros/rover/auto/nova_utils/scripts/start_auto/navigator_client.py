#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from rclpy.action import ActionClient
from rclpy.client import Client
from rclpy.qos import QoSProfile, QoSDurabilityPolicy, QoSPresetProfiles
from rclpy.executors import MultiThreadedExecutor
from std_msgs.msg import String
from geometry_msgs.msg import PoseStamped, Point
from geographic_msgs.msg import GeoPoint
from nav2_msgs.action import NavigateThroughPoses
from nova_interfaces.msg import Status
from nova_interfaces.action import URCThroughPoses
from nova_interfaces.srv import CartographerCommand, RGBInput
from action_msgs.msg import GoalStatus
from visualization_msgs.msg import Marker, MarkerArray
from robot_localization.srv import FromLL
from tf2_ros import Buffer, TransformListener
from tf_transformations import quaternion_from_euler
import json
import os
import sys
import time
import math
from typing import Tuple
from os.path import expanduser

class NavigatorClient():
    def __init__(self, node:Node):
        # Set parameters
        self.node=node
        self.goal_handle=None
        self.status=None

        # Create action client for URCThroughPoses
        self.urc_navigator_client = ActionClient(self, URCThroughPoses, '/urc_through_poses')
        self.get_logger().info('Waiting for /urc_through_poses...')
        if not self.navigator_client.wait_for_server(timeout_sec=10.0):
            self.get_logger().error('/urc_through_poses not available! Exiting.')
            return
        self.get_logger().info('/urc_through_poses available.')

        # Create action client for /navigate_through_poses
        self.nav2_navigator_client = ActionClient(self, NavigateThroughPoses, '/navigate_through_poses')
        self.get_logger().info('Waiting for /navigate_through_poses...')
        if not self.action_client.wait_for_server(timeout_sec=10.0):
            self.get_logger().error('/navigate_through_poses not available! Exiting.')
            return
        self.get_logger().info('/navigate_through_poses available.')

        # Create action client for /urc_through_poses
        self.navigator_client = ActionClient(self, URCThroughPoses, '/urc_through_poses')
        self.get_logger().info('Waiting for /urc_through_poses...')
        if not self.navigator_client.wait_for_server(timeout_sec=10.0):
            self.get_logger().error('/urc_through_poses not available! Exiting.')
            return
        self.get_logger().info('/urc_through_poses available.')

        # Create subscriber for waypoints and navigation status
        self._sub_blackboard = self.create_subscription(String, '/blackboard', self.blackboard_callback, QoSPresetProfiles.SENSOR_DATA.value)
        self.get_logger().info('Successfully subscribed to /blackboard.')

        # Create publisher for navigation status
        self._status_pub = self.create_publisher(Status, self._status_topic, QoSPresetProfiles.SENSOR_DATA.value)
        self.get_logger().info(f'Successfully publishing to {self._status_topic} created!')

    def start(type, poses, search_radius)
        match type:

            case CartographerCommand.GNSS:
                self.go_to_gps(poses)
                return

            case CartographerCommand.AR:
                self.go_to_ar_tag(poses)
                return

            case CartographerCommand.OBJECT:
                self.go_to_object(poses, search_radius)
                return

    def go_to_gps(poses)
        goal_action = NavigateThroughPoses.Goal()
        goal_action.poses = poses
        goal_action.behaviour_tree = f"{expanduser("~")}/nova/src/ros/rover/auto/nova_behavior_tree/behavior_tree/shared/nav_through_poses_remove_nearby_collision_goals.xml"
        self.node.get_logger().info('Sending waypoints to /navigate_through_poses...')
        self.call(goal_action)

    def go_to_ar_tag(poses)
        goal_action = URCThroughPoses.Goal()
        goal_action.poses = poses
        goal_action.behaviour_tree = f"{expanduser("~")}/nova/src/ros/rover/auto/nova_behavior_tree/behavior_tree/urc/urc_through_poses_aruco.xml"
        goal_action.search_radius = search_radius
        self.node.get_logger().info('Sending waypoints to /urc_through_poses...')
        self.call(goal_action)

    def go_to_object(poses, search_radius)
        goal_action = URCThroughPoses.Goal()
        goal_action.poses = poses
        goal_action.behaviour_tree = f"{expanduser("~")}/nova/src/ros/rover/auto/nova_behavior_tree/behavior_tree/urc/urc_through_poses_object.xml"
        goal_action.search_radius = search_radius
        self.node.get_logger().info('Sending waypoints to /urc_through_poses...')
        self.call(goal_action)

    def call(self, goal_action):
        '''Sends the waypoints asynchronously to the URCThroughPoses action server.'''
        send_future = self.navigator_client.send_goal_async(goal_action)
        send_future.add_done_callback(self.response)

    def response(self, future):
        goal_handle = future.result()
        if not goal_handle.accepted:
            self.get_logger().error('Goal rejected!.')
            return

        self.get_logger().info('Goal accepted.')
        self.goal_handle = goal_handle

        # Set up asynchronous result callback
        result_future = goal_handle.get_result_async()
        result_future.add_done_callback(self.result)

    def result(self, future):
        result = future.result()
        self.status = result.status

    def finished(self):
        return self.status is not None