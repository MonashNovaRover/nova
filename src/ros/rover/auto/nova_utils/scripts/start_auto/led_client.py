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

class LEDClient():
    def __init__(self, node:Node):
        # Set parameters
        self.node = node

        # 📝 Create service client for LED control
        self.led_client = self.node.create_client(RGBInput, '/set_RGBInput')
        if not self.led_client.wait_for_service(timeout_sec=10.0):
            self.node.get_logger().error('Service /set_RGBInput not available. Cannot change LED color.')
            return
        self.node.get_logger().info('Service /set_RGBInput available!')

    def red():
        self.call((255, 0, 0), False)

    def blue():
        self.call((0, 0, 255), False)

    def green():
        self.call((0, 255, 0), True)

    def call(self, rgb: Tuple[int, int, int], flash: bool) -> bool:
        '''Calls the set_RGBInput service to change the LED color.'''
        request = RGBInput.Request()
        request.r = rgb[0]
        request.g = rgb[1]
        request.b = rgb[2]
        request.flash = flash
        future = self.led_client.call_async(request)
        future.add_done_callback(self.result)

    def result(self, future):
        '''Callback for the set_RGBInput service call.'''
        try:
            response = future.result()
            if response.success:
                self.node.get_logger().info('LED called and changed successfully.')
            else:
                self.node.get_logger().error('LED called but failed to change!')
        except Exception as e:
            self.node.get_logger().error(f'LED call failed: {e}')