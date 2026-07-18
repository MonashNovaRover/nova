#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose:
Convert a NavSatFix message from /gps/filtered and a heading from /odometry/global
into a custom GPSData message published on /gps_rover/fix_custom.

NODE: gps_filtered_heading_to_custom
TOPICS:
  - subscriber: /gps/filtered          [NavSatFix]
  - subscriber: /odometry/global       [Odometry]
  - publisher:  /gps_rover/fix_custom  [GPSData]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_utils
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

import math

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSPresetProfiles
from nav_msgs.msg import Odometry
from sensor_msgs.msg import NavSatFix
from nova_interfaces.msg import GPSData


def quaternion_to_yaw(x: float, y: float, z: float, w: float) -> float:
    siny_cosp = 2.0 * (w * z + x * y)
    cosy_cosp = 1.0 - 2.0 * (y * y + z * z)
    return math.atan2(siny_cosp, cosy_cosp)


def yaw_rad_to_heading_deg(yaw_rad: float) -> float:
    yaw_deg = math.degrees(yaw_rad)
    heading = (90.0 - yaw_deg) % 360.0
    if heading < 0.0:
        heading += 360.0
    return heading


class GpsFilteredHeadingToCustom(Node):
    def __init__(self):
        super().__init__('gps_filtered_heading_to_custom')

        self.pose_custom = GPSData()

        self.gps_sub = self.create_subscription(
            NavSatFix,
            '/gps/filtered',
            self.gps_callback,
            QoSPresetProfiles.SENSOR_DATA.value,
        )
        self.odom_sub = self.create_subscription(
            Odometry,
            '/odometry/global',
            self.odom_callback,
            QoSPresetProfiles.SENSOR_DATA.value,
        )
        self.fix_custom_pub = self.create_publisher(GPSData, '/gps_rover/fix_custom', QoSPresetProfiles.SENSOR_DATA.value)

    def gps_callback(self, msg: NavSatFix):
        self.pose_custom.header = msg.header
        self.pose_custom.status = msg.status
        self.pose_custom.latitude = msg.latitude
        self.pose_custom.longitude = msg.longitude
        self.pose_custom.altitude = msg.altitude
        self.publish_custom_fix()

    def odom_callback(self, msg: Odometry):
        self.pose_custom.heading = yaw_rad_to_heading_deg(
            quaternion_to_yaw(
                msg.pose.pose.orientation.x,
                msg.pose.pose.orientation.y,
                msg.pose.pose.orientation.z,
                msg.pose.pose.orientation.w,
            )
        )
        self.publish_custom_fix()

    def publish_custom_fix(self):
        self.fix_custom_pub.publish(self.pose_custom)


def main(args=None):
    rclpy.init(args=args)
    node = GpsFilteredHeadingToCustom()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
