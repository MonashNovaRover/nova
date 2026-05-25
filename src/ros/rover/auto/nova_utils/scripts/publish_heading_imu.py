#!/usr/bin/env python3

import math

import rclpy
from rclpy.node import Node

from sensor_msgs.msg import Imu
from geometry_msgs.msg import Quaternion


def yaw_to_quaternion(yaw_rad: float) -> Quaternion:
    """
    Convert yaw around Z axis to quaternion.

    Roll = 0
    Pitch = 0
    Yaw = yaw_rad
    """
    q = Quaternion()
    q.x = 0.0
    q.y = 0.0
    q.z = math.sin(yaw_rad / 2.0)
    q.w = math.cos(yaw_rad / 2.0)
    return q


class HeadingToImuOnce(Node):
    def __init__(self):
        super().__init__('heading_to_imu_once')

        self.declare_parameter('heading_degrees', 0.0)
        self.declare_parameter('topic', '/gps_rover/heading_imu')

        self.heading_degrees = (
            self.get_parameter('heading_degrees')
            .get_parameter_value()
            .double_value
        )

        topic = (
            self.get_parameter('topic')
            .get_parameter_value()
            .string_value
        )

        self.publisher = self.create_publisher(Imu, topic, 10)

        self.frame_id = 'base_link'

        # Short delay so the publisher has time to match subscribers.
        self.timer = self.create_timer(0.2, self.publish_once)

        self.has_published = False

    def publish_once(self):
        if self.has_published:
            return

        yaw_rad = math.radians(self.heading_degrees)

        msg = Imu()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = self.frame_id

        msg.orientation = yaw_to_quaternion(yaw_rad)

        # Orientation covariance.
        # Set roll/pitch unknown-ish, yaw reasonably trusted.
        # Adjust these depending on how much you trust the heading.
        msg.orientation_covariance = [
            999999.0, 0.0,      0.0,
            0.0,      999999.0, 0.0,
            0.0,      0.0,      0.01,
        ]

        # Mark angular velocity and linear acceleration as unavailable.
        msg.angular_velocity_covariance[0] = -1.0
        msg.linear_acceleration_covariance[0] = -1.0

        self.publisher.publish(msg)

        self.get_logger().info(
            f'Published IMU orientation once: heading={self.heading_degrees:.3f} deg, '
            f'yaw={yaw_rad:.3f} rad, topic={self.publisher.topic_name}'
        )

        self.has_published = True

        # Shut down shortly after publishing.
        self.destroy_timer(self.timer)
        rclpy.shutdown()


def main(args=None):
    rclpy.init(args=args)
    node = HeadingToImuOnce()
    rclpy.spin(node)


if __name__ == '__main__':
    main()