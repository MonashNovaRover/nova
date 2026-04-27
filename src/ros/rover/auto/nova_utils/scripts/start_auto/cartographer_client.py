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
from nova_interfaces.action import URC2025Navigator
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

class CartographerClient():
    def __init__(self, node:Node):
        self.goals=[]
        self.type=None
        self.search_radius=None

        # Create service for Auto URC Cartographer GUI 
        self.cartographer_service = self.create_service(CartographerCommand, '/autonomous/cartographer_command', self.called)
        self.get_logger().info('Serving /autonomous/cartographer_command.')

    def called(self, request, response):
        '''Loads waypoints from GUI and converts them into PoseStamped messages.'''
        self.goals = request.goals
        self.type = request.type
        self.search_radius = request.search_radius
        while not self.response:
            pass
        return response

    def received_goals(self):
        return len(self.goals) > 0

    def respond(self, response:bool):
        self.response = response