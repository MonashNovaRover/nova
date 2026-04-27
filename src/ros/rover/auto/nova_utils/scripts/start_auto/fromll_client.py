#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Node that loads waypoints from a file and 
sends it to the /urc_2025_navigator action
server. It continuously checks the status of the 
action server to monitor if it has been
aborted. If the action server has aborted, it will 
reload the waypoints to restart navigation.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: WaypointNavigator
TOPICS:
    - subscriber: /blackboard                   [std_msgs/msg/String]
    - publisher: /auto/status             [nova_interfaces/msg/Status]
SERVICES:
    - client: /fromLL                           [robot_localization/srv/FromLL]
    - client: /set_RGBInput                     [nova_interfaces/srv/RGBInput]
    - service: /autonomous/cartographer_command [nova_interfaces/srv/CartographerCommand]
ACTIONS: 
  - client: /urc_2025_navigator                 [URCThroughPoses]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	nova_utils
AUTHOR(S):	Tarik Thomas, Terry Tian, 
            Victor Bartlinski
CREATION:	27/05/2025
EDITED:		29/05/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
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
from sensor_msgs.msg import NavSatFix
import json
import os
import sys
import time
import math
from typing import Tuple

class FromLLClient():
    def __init__(self, node:Node):
        # Set parameters
        self.node=node
        self.called=False
        self.lls=[]
        self.ll=None
        self.poses=[]

        # Create service client for robot_localization /fromLL
        self.fromll_client = self.node.create_client(FromLL, '/fromLL')
        self.get_logger().info('Waiting for /fromLL server...')
        if not self.fromll_client.wait_for_service(timeout_sec=10.0):
            self.get_logger().error('Service /fromLL not available! Exiting.')
            return
        self.get_logger().info('Service /fromLL available.')

    def call(self):
        '''Converts GNSS goal to a geometry_msgs/msg/Point using the robot_localization FromLL service.'''
        self.called = True
        fromll_req = FromLL.Request()
        fromll_req.ll_point.latitude = self.ll.lat
        fromll_req.ll_point.longitude = self.ll.lon
        self.get_logger().info(f'Sending GNSS goal {lat}, {lon} to /fromLL...')
        future = self._fromll_client.call_async(fromll_req)
        future.add_done_callback(self.result)
    
    def result(self, future):
        result = future.result()
        pose = self.point_to_pose(result.map_point)
        self.poses.append(pose)
        self.called = False

    def point_to_pose(self, point):
        '''Creates a waypoint from a geometry_msgs/msg/Point.'''
        pose = PoseStamped()
        pose.header.frame_id = 'map'
        pose.header.stamp = self.get_clock().now().to_msg()
        pose.pose.position.x = point.x
        pose.pose.position.y = point.y
        pose.pose.position.z = 0.0
        return pose

    def lls_to_poses(self, lls:list[NavSatFix])
        self.lls = iter(lls)

    def waiting(self)
        return self.called

    def has_next(self)
        self.ll = next(self.lls, None)
        if self.ll is None:
            return True
        return False