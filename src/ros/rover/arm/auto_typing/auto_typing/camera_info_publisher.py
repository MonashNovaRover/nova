#!/usr/bin/env python3

"""Publishes CameraInfo messages required by aruco_opencv for the auto typing camera"""

from sensor_msgs.msg import CameraInfo

import math

import rclpy
from rclpy.node import Node


CAMERA_INFO_TOPIC = '/arm/camera_info'


class CameraInfoPublisher(Node):
    def __init__(self):
        super().__init__('camera_info_publisher')

        self.camera_info_topic = self.declare_parameter(
            'camera_info_topic', CAMERA_INFO_TOPIC
        ).get_parameter_value().string_value

        self.frame_id = self.declare_parameter(
            'frame_id', 'image_frame'
        ).get_parameter_value().string_value

        self.width = self.declare_parameter(
            'image_width', 1280
        ).get_parameter_value().integer_value

        self.height = self.declare_parameter(
            'image_height', 720
        ).get_parameter_value().integer_value

        hfov = self.declare_parameter(
            'hfov', 61.3727248
        ).get_parameter_value().double_value

        dist_arr = list(self.declare_parameter(
            'distortion_matrix', [0.000477749236441667163, -0.06869748182906846, -0.0030440664969761, 0.00015872921312327083, -0.35803596544161447]
        ).get_parameter_value().double_array_value)

        publish_rate = self.declare_parameter(
            'publish_rate', 10.0
        ).get_parameter_value().double_value

        focal_length = self.width / 2.0 / math.tan(math.radians(hfov) / 2.0)
        cx = self.width / 2.0
        cy = self.height / 2.0

        self.camera_info_msg = CameraInfo()
        self.camera_info_msg.header.frame_id = self.frame_id
        self.camera_info_msg.width = self.width
        self.camera_info_msg.height = self.height

        # plumb_bob is the standard distortion model for normal pinhole cameras
        self.camera_info_msg.distortion_model = 'plumb_bob'
        self.camera_info_msg.d = [float(x) for x in dist_arr]

        # Intrinsic camera matrix
        self.camera_info_msg.k = [
            float(focal_length), 0.0, float(cx),
            0.0, float(focal_length), float(cy),
            0.0, 0.0, 1.0,
        ]

        # Rectification matrix
        self.camera_info_msg.r = [
            1.0, 0.0, 0.0,
            0.0, 1.0, 0.0,
            0.0, 0.0, 1.0,
        ]

        # Projection/camera matrix
        self.camera_info_msg.p = [
            float(focal_length), 0.0, float(cx), 0.0,
            0.0, float(focal_length), float(cy), 0.0,
            0.0, 0.0, 1.0, 0.0,
        ]

        self.pub = self.create_publisher(CameraInfo, self.camera_info_topic, 10)
        self.timer = self.create_timer(1.0 / publish_rate, self.publish_camera_info)

        self.get_logger().info(
            f"Publishing CameraInfo on {self.camera_info_topic} "
            f"for {self.width}x{self.height}, frame_id={self.frame_id}"
        )

    def publish_camera_info(self) -> None:
        msg = CameraInfo()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = self.camera_info_msg.header.frame_id
        msg.width = self.camera_info_msg.width
        msg.height = self.camera_info_msg.height
        msg.distortion_model = self.camera_info_msg.distortion_model
        msg.d = list(self.camera_info_msg.d)
        msg.k = list(self.camera_info_msg.k)
        msg.r = list(self.camera_info_msg.r)
        msg.p = list(self.camera_info_msg.p)
        self.pub.publish(msg)


def main():
        rclpy.init()
        node = CameraInfoPublisher()
        rclpy.spin(node)
        rclpy.shutdown()


if __name__ == '__main__':
    main()