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

from geometry_msgs.msg import PoseStamped, TransformStamped, Transform, Vector3, Quaternion
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
        self.param_base_link_rate = self.declare_parameter("base_link_pub_rate_hz", 30).value
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

        self.pub_t265_forward_frame = self.create_publisher(PoseStamped, "/localisation/t265_forward_frame", 10)
        self.pub_t265_frame = self.create_publisher(PoseStamped, "/localisation/t265_frame", 10)

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
    
    def transform_t265_pose_to_base_pose(self, t265_tf: Transform) -> Transform:
        """
        Transform the position and orientation of the t265 to our frame"""
        try:
            t265_forward_to_t265_footprint : Transform = self.tf_buffer.lookup_transform("t265_forward", "t265_footprint", Time()).transform
            t265_to_t265_footprint : Transform = self.tf_buffer.lookup_transform("t265", "t265_footprint", Time()).transform
            base_link_to_t265_footprint : Transform = self.tf_buffer.lookup_transform("base_link", "t265_footprint", Time()).transform
        except Exception as e:
            self.get_logger().warn(f"Failed to lookup transform: {e}")
            return None

        return_transform = Transform()
        return_transform.rotation = self.transform_t265_orientation(t265_tf, t265_forward_to_t265_footprint, t265_to_t265_footprint)
        return_transform.translation = self.transform_t265_position(t265_tf, return_transform, base_link_to_t265_footprint)

        return return_transform

    def transform_t265_position(self, t265_tf: Transform, non_offset_transform: Transform, base_link_to_t265_footprint: Transform) -> Vector3:
        """
        Translate and rotate the position point to our frame
        """
        position = Vector3()

        translation_point = np.array([t265_tf.translation.x, t265_tf.translation.y, t265_tf.translation.z])
        non_offset_transform.translation.x, non_offset_transform.translation.y, non_offset_transform.translation.z = \
            transform.transform_from_quat(base_link_to_t265_footprint.rotation, translation_point)

        self.get_logger().debug(f"t265 position: {translation_point}")
        self.get_logger().debug(f"transformed position: {position}")


        # transform to offset frame
        external_point = -np.array([base_link_to_t265_footprint.translation.x, base_link_to_t265_footprint.translation.y, base_link_to_t265_footprint.translation.z])
        
        self.get_logger().debug(f"external point: {external_point}")

        # do transform
        transformed_point = transform.transform_points(non_offset_transform, external_point).flatten()

        self.get_logger().debug(f"transformed point: {transformed_point}")

        # undo transformed offset to get back to original frame
        position.x, position.y, position.z = transformed_point - external_point
        return position

    def transform_t265_orientation(self, t265_tf: Transform, t265_forward_to_t265_footprint: Transform, t265_to_t265_footprint: Transform) -> Quaternion:
        """
        Rotate the t265 orientation from its own frame to our frame
        """
        p, r, y = transform.quat_to_euler(t265_tf.rotation)
        _, roll_offset, _ = transform.quat_to_euler(t265_to_t265_footprint.rotation)
        r += roll_offset
        flat_rotation = transform.euler_to_quat((p, r, y))

        tmp = transform.quaternion_multiply(t265_forward_to_t265_footprint.rotation, flat_rotation)
        base_link_orientation = transform.quaternion_right_divide(tmp, t265_forward_to_t265_footprint.rotation)
        self.get_logger().debug(f"t265 transform: {t265_tf}")
        self.get_logger().debug(f"t265_forward -> footprint: {t265_forward_to_t265_footprint}")
        self.get_logger().debug(f"p, r, y: {p}, {r}, {y}")
        self.get_logger().debug(f"roll offset: {roll_offset}")
        self.get_logger().debug(f"tmp: {tmp}")
        self.get_logger().debug(f"Base link orientation: {base_link_orientation}")
        return base_link_orientation

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

        transformed = self.transform_t265_pose_to_base_pose(t265_transform)
        if transformed is not None:
            base_link_transform.transform = transformed
        else:
            return None
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
        if tf_stamped is not None:
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
