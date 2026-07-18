#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose:
Convert a Gazebo IMU heading topic from ENU yaw convention
(0 rad = East) to north-zero heading convention (0 rad = North).
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gz_heading_imu_fixer
TOPICS:
  - subscriber: /gps_rover/heading_imu_raw [Imu]
  - publisher: /gps_rover/heading_imu      [Imu]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_utils
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

import math

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Imu


def quaternion_to_rpy(x: float, y: float, z: float, w: float):
    sinr_cosp = 2.0 * (w * x + y * z)
    cosr_cosp = 1.0 - 2.0 * (x * x + y * y)
    roll = math.atan2(sinr_cosp, cosr_cosp)

    sinp = 2.0 * (w * y - z * x)
    if abs(sinp) >= 1.0:
        pitch = math.copysign(math.pi / 2.0, sinp)
    else:
        pitch = math.asin(sinp)

    siny_cosp = 2.0 * (w * z + x * y)
    cosy_cosp = 1.0 - 2.0 * (y * y + z * z)
    yaw = math.atan2(siny_cosp, cosy_cosp)

    return roll, pitch, yaw


def rpy_to_quaternion(roll: float, pitch: float, yaw: float):
    cy = math.cos(yaw * 0.5)
    sy = math.sin(yaw * 0.5)
    cp = math.cos(pitch * 0.5)
    sp = math.sin(pitch * 0.5)
    cr = math.cos(roll * 0.5)
    sr = math.sin(roll * 0.5)

    w = cr * cp * cy + sr * sp * sy
    x = sr * cp * cy - cr * sp * sy
    y = cr * sp * cy + sr * cp * sy
    z = cr * cp * sy - sr * sp * cy
    return x, y, z, w


class GzHeadingImuFixer(Node):
    def __init__(self):
        super().__init__('gz_heading_imu_fixer')

        self.sub_topic = self.declare_parameter('sub_topic', '/gps_rover/heading_imu_raw').value
        self.pub_topic = self.declare_parameter('pub_topic', '/gps_rover/heading_imu').value
        # Convert ENU yaw (east-zero) to north-zero heading convention.
        self.yaw_offset_rad = self.declare_parameter('yaw_offset_rad', -1.57079632679).value

        self.sub_imu = self.create_subscription(Imu, self.sub_topic, self.sub_callback, 10)
        self.pub_imu = self.create_publisher(Imu, self.pub_topic, 10)

    def sub_callback(self, msg: Imu):
        out = Imu()
        out.header = msg.header
        out.orientation = msg.orientation
        out.orientation_covariance = msg.orientation_covariance
        out.angular_velocity = msg.angular_velocity
        out.angular_velocity_covariance = msg.angular_velocity_covariance
        out.linear_acceleration = msg.linear_acceleration
        out.linear_acceleration_covariance = msg.linear_acceleration_covariance

        roll, pitch, yaw = quaternion_to_rpy(
            msg.orientation.x,
            msg.orientation.y,
            msg.orientation.z,
            msg.orientation.w,
        )
        yaw += float(self.yaw_offset_rad)

        x, y, z, w = rpy_to_quaternion(roll, pitch, yaw)
        out.orientation.x = x
        out.orientation.y = y
        out.orientation.z = z
        out.orientation.w = w

        self.pub_imu.publish(out)


def main(args=None):
    rclpy.init(args=args)
    node = GzHeadingImuFixer()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
