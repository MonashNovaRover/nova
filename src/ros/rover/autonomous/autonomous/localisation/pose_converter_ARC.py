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
from rclpy.duration import Duration

from geometry_msgs.msg import PoseStamped, TransformStamped, Transform, Vector3
from tf2_ros import TransformBroadcaster, StaticTransformBroadcaster, TransformListener, Buffer

import autonomous.math_utils.transform as transform
from autonomous.config.runtime_params import pose_file

import numpy as np
import time
import logging


class PoseConverter(Node):

    def __init__(self):
        super().__init__("pose_converter")
        # way-point publisher publishes a bunch of waypoints at once (hence using the 2D map datatype
        self.get_logger().set_level(logging.INFO)

        self.param_do_ORB_SLAM3 = self.declare_parameter("do_orbslam", False).value
        self.param_base_link_rate = self.declare_parameter("base_link_pub_rate_hz", 20).value
        self.param_use_euler_angles = self.declare_parameter("use_euler", False).value
        self.param_initial_quat = self.declare_parameter("initial_base_link_quat", [0., 0., 0., 0., 0., 0., 1.]).value
        self.param_initial_euler = self.declare_parameter("initial_base_link_euler", [0., 0., 0., 0., 0., 0.]).value

        self.tf_base_link = TransformBroadcaster(self)
        self.tf_initial_offset = StaticTransformBroadcaster(self)
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, node=self, spin_thread=True)
        self.last_pose = None

        pose_topic = "/slam/pose" if self.param_do_ORB_SLAM3 else "/T265/pose"
        self.sub_pose = self.create_subscription(PoseStamped, pose_topic, self.callback_t265, 10)
        # current state of internal message

        self.get_logger().info("Waiting for transform from 'base_link' to 't265'...")
        while not self.tf_buffer.can_transform('base_link', 't265', Time()):
            time.sleep(0.1)

        self.get_initial_transform()
        self.create_timer(1 / self.param_base_link_rate, self.timer_callback)
        self.get_logger().info("Pose converter node up!")

    def fill_initial_pose(self, initial_transform: Transform):
        """
        Read data from params and fill transform with it
        """
        if self.param_use_euler_angles:
            try:
                initial_transform.translation.x, initial_transform.translation.y, initial_transform.translation.z = \
                    self.param_initial_euler[0], self.param_initial_euler[1], self.param_initial_euler[2]
                initial_transform.rotation = transform.quat_to_euler(
                    self.param_initial_euler[3],
                    self.param_initial_euler[4],
                    self.param_initial_euler[5]
                )
            except IndexError as e:
                self.get_logger().error(f"Incorrect euler angle layout in parameter: {e}")
            except Exception as e:
                self.get_logger().error(f"Failed to read parameter initial pose {e}")
        else:
            try:
                initial_transform.translation.x, initial_transform.translation.y, initial_transform.translation.z = \
                    self.param_initial_quat[0], self.param_initial_quat[1], self.param_initial_quat[2]
                initial_transform.rotation.x, initial_transform.rotation.y, initial_transform.rotation.z, initial_transform.rotation.w = \
                    self.param_initial_quat[3], self.param_initial_quat[4], self.param_initial_quat[5], self.param_initial_quat[6]
            except IndexError as e:
                self.get_logger().error(f"Incorrect quaternion layout in parameter: {e}")
            except Exception as e:
                self.get_logger().error(f"Failed to read parameter initial pose {e}")

        return initial_transform

    def get_initial_transform(self):
        """
        If we want to, load initial rover position from file
        """
        initial_transform = TransformStamped()
        initial_transform.header.frame_id = 'map'
        initial_transform.header.stamp = self.get_clock().now().to_msg()
        initial_transform.child_frame_id = 'initial_base_link'
        initial_transform.transform = self.fill_initial_pose(initial_transform.transform)

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


    def callback_t265(self, msg: PoseStamped):
        """
        Take T265 messages, offset them to get the rover's pose, and save the pose estimate
        """
        self.last_pose = msg

    def translate_to_base_link(self, t265_pose: PoseStamped):
        """
        Translates the t265 camera's offset into a base link offset. Requires the following steps:
            1. Get base link to t265 static transform
            2. Calculate the new t265 pose in the base link frame
            3. Calculate the orientation of the t265 in the initial base link frame by dividing these two quaternions
            4. Rotate the base link to t265 offset by that orientation, then subtract it from the t265 pose in the 
                base link frame to get the base link pose
        """
        # Static transform from base_link to t265 frame
        try:
            t265_offset = self.tf_buffer.lookup_transform('base_link', 't265', Time()).transform
            t265_flattening_offset = self.tf_buffer.lookup_transform('base_link', 't265_flattening', Time()).transform
        except Exception as e:
            self.get_logger().warn(str(e), once=True)
            return

        # Current t265 pose in base_link frame
        t265_transform = Transform()
        t265_transform.translation.x = t265_pose.pose.position.x
        t265_transform.translation.y = t265_pose.pose.position.y
        t265_transform.translation.z = t265_pose.pose.position.z
        t265_transform.rotation = t265_pose.pose.orientation

        base_link_transform = TransformStamped()
        base_link_transform.transform = transform.offset_transform(t265_transform, t265_offset)
        base_link_transform.header.stamp = t265_pose.header.stamp
        base_link_transform.header.frame_id = 'initial_base_link'
        base_link_transform.child_frame_id = 'base_link'
        self.get_logger().debug(f"initial base link to base_link: {base_link_transform}")

        return base_link_transform

    def timer_callback(self):
        """
        Called every timer_period. Publishes base link transform
        :return:
        """
        if self.last_pose is None:
            self.get_logger().debug("Exiting localisation! No pose received")
            return

        tf_stamped : TransformStamped = self.translate_to_base_link(self.last_pose)
        self.get_logger().debug(f"Transformed to base_link: \n{self.last_pose}\n{tf_stamped}")
        self.tf_base_link.sendTransform(tf_stamped)


def main():
    rclpy.init()
    node = PoseConverter()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
