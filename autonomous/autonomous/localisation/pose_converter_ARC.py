#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Takes pose estimates from either the
T265 internal SLAM algorithm or our ORB_SLAM3
algorithm, transforms to the base_link frame, and 
publishes the base_link transform
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: pose_converter
TOPICS:
  - subscriber: /slam/pose [PoseStamped]
  - subscriber: /T265/pose [PoseStamped]
  - broadcaster: 'base_link' [TransformStamped]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    autonomous
AUTHOR(S):	Max Tory
CREATION:	04/03/2023
EDITED:		04/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - subscribe to T265 slam or ORB_SLAM3 depending
    on parameter
 - Transform frames from T265 frame to base_link
 - Publish Base Link TransformStamped
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.time import Time

from geometry_msgs.msg import PoseStamped, TransformStamped, Transform
from tf2_ros import TransformBroadcaster, StaticTransformBroadcaster, TransformListener, Buffer

import autonomous.math_utils.transform as transform
from autonomous.config.runtime_params import pose_file

import numpy as np
import time


class TemplateNode(Node):

    def __init__(self):
        super().__init__("TemplateNode")
        # way-point publisher publishes a bunch of waypoints at once (hence using the 2D map datatype
        self.param_do_ORB_SLAM3 = self.declare_parameter("do_orbslam", False).value
        self.param_base_link_rate = self.declare_parameter("base_link_pub_rate_hz", 20).value
        self.param_load_pose_file = self.declare_parameter("load_file_pose", False).value

        self.tf_base_link = TransformBroadcaster(self)
        self.tf_initial_offset = StaticTransformBroadcaster(self)
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, node=self, spin_thread=True)
        self.last_pose = None

        pose_topic = "/slam/pose" if self.param_do_ORB_SLAM3 else "/T265/pose"
        self.sub_pose = self.create_subscription(TransformStamped, pose_topic, self.pose_callback, 10)
        # current state of internal message

        self.get_logger().info("Waiting for transform from 'map' to 'base_link'...")
        while not self.tf_buffer.can_transform('map', 'base_link', Time()):
            time.sleep(0.1)
        self.get_logger().info("Found Transform!", once=True)

        self.create_timer(self.param_base_link_rate, self.broadcast_base_link)

    def fill_initial_pose(self, transform: Transform):
        """
        Read data from file and fill transform with it
        """
        try:
            pose = np.loadtxt(pose_file).reshape(4)
        except FileNotFoundError as e:
            self.get_logger().warn("Couldn't find file!")

        transform.translation.x, transform.translation.y, transform.translation.z = pose[:3]
        # filling in yaw
        transform.rotation.z = np.sin(pose[3] / 2)
        transform.rotation.w = np.cos(pose[3] / 2)
        return transform

    def get_initial_transform(self):
        """
        If we want to, load initial rover position from file
        """
        # TODO: set in param somewhere?
        initial_transform = TransformStamped()
        initial_transform.header.frame_id = 'map'
        initial_transform.header.stamp = self.get_clock().now().to_msg()
        initial_transform.child_frame_id = 'initial_base_link'
        if self.param_load_pose_file:
            initial_transform.transform = self.fill_initial_pose(initial_transform.transform)
        else:
            initial_transform.transform.rotation.w = 1.0

        self.tf_initial_offset.sendTransform(initial_transform)

        base_link_transform = TransformStamped()
        base_link_transform.header.frame_id = 'initial_base_link'
        base_link_transform.header.stamp = self.get_clock().now().to_msg()
        base_link_transform.child_frame_id = 'base_link'

        base_link_transform.transform.rotation.w = 1.0
        base_link_transform.transform.rotation.x = 0.0
        base_link_transform.transform.rotation.y = 0.0
        base_link_transform.transform.rotation.z = 0.0
        self.tf_base_link.sendTransform(base_link_transform)


    def callback_t265(self, msg: TransformStamped):
        """
        Take T265 messages, offset them to get the rover's pose, and save the pose estimate
        """
        self.last_pose = msg

    def translate_to_base_link(self, t265_transform: TransformStamped):
        base_link_transform = TransformStamped()
        base_link_transform.header.stamp = self.get_clock().now().to_msg()
        base_link_transform.header.frame_id = 'initial_base_link'
        base_link_transform.child_frame_id = 'base_link'

        try:
            t265_offset = self.tf_buffer.lookup_transform('base_link', 't265', Time()).transform
        except Exception as e:
            self.get_logger().warn(str(e), once=True)
            return
        base_link_transform.transform = transform.offset_transform(transform=t265_transform, offset=t265_offset)
        return base_link_transform

    def timer_callback(self):
        """
        Called every timer_period. Publishes base link transform
        :return:
        """
        if self.last_pose is None:
            return
        tf_stamped : TransformStamped = self.translate_to_base_link(self.last_pose)
        self.tf_base_link.sendTransform(tf_stamped)


def main():
    rclpy.init()
    template_node = TemplateNode()
    rclpy.spin(template_node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
