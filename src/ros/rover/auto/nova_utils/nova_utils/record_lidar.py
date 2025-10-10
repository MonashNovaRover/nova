#!/usr/bin/env python3
import argparse

import numpy as np
import rerun as rr

import cv_bridge
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile
from rclpy.time import Time
from sensor_msgs.msg import Image, PointCloud2, Imu
from nav_msgs.msg import Odometry
from sensor_msgs_py import point_cloud2
from numpy.lib.recfunctions import structured_to_unstructured


class DataStreamRecorder(Node):
    def __init__(self) -> None:
        super().__init__("data_stream_recorder")

        rr.init("data_stream_recorder",spawn=False)

        self.cv_bridge = cv_bridge.CvBridge()

        # QoS for sensors
        qos_profile = QoSProfile(depth=10)


        # Odometry
        self.odom_sub = self.create_subscription(
            Odometry,
            "/pivot_drive_controller/odom",
            self.odom_callback,
            qos_profile,
        )

        # LiDAR Points
        self.lidar_sub = self.create_subscription(
            PointCloud2,
            "/livox/lidar/points",
            self.lidar_callback,
            qos_profile,
        )

        # IMU
        self.imu_sub = self.create_subscription(
            Imu,
            "/oak/imu/transformed",
            self.imu_callback,
            qos_profile,
        )

        # RGB Image
        self.rgb_sub = self.create_subscription(
            Image,
            "/oak/rgb/image_raw",
            self.rgb_callback,
            qos_profile,
        )

        # Depth Image
        self.depth_sub = self.create_subscription(
            Image,
            "/oak/depth/image_raw",
            self.depth_callback,
            qos_profile,
        )

    def odom_callback(self, odom: Odometry) -> None:
        """Log odometry data."""
        time = Time.from_msg(odom.header.stamp)
        rr.set_time("ros_time", np.datetime64(time.nanoseconds, "ns"))

        position = odom.pose.pose.position
        orientation = odom.pose.pose.orientation

        rr.log(
            "odom/pose",
            rr.Transform3D(
                translation=[position.x, position.y, position.z],
                rotation=rr.Quaternion(xyzw=[orientation.x, orientation.y, orientation.z, orientation.w]),
            ),
        )

        rr.log("odom/linear_vel", rr.Scalars(odom.twist.twist.linear.x))
        rr.log("odom/angular_vel", rr.Scalars(odom.twist.twist.angular.z))

    def lidar_callback(self, points: PointCloud2) -> None:
        """Log LiDAR point cloud."""
        time = Time.from_msg(points.header.stamp)
        rr.set_time("ros_time", np.datetime64(time.nanoseconds, "ns"))

        pts = point_cloud2.read_points(points, field_names=["x", "y", "z"], skip_nans=True)
        pts = structured_to_unstructured(pts)
        rr.log("lidar/points", rr.Points3D(pts))

    def imu_callback(self, imu: Imu) -> None:
        """Log IMU data."""
        time = Time.from_msg(imu.header.stamp)
        rr.set_time("ros_time", np.datetime64(time.nanoseconds, "ns"))

        rr.log("imu/angular_velocity", rr.Vectors([imu.angular_velocity.x, imu.angular_velocity.y, imu.angular_velocity.z]))
        rr.log("imu/linear_acceleration", rr.Vectors([imu.linear_acceleration.x, imu.linear_acceleration.y, imu.linear_acceleration.z]))

        orientation = imu.orientation
        rr.log(
            "imu/orientation",
            rr.Quaternion(xyzw=[orientation.x, orientation.y, orientation.z, orientation.w]),
        )

    def rgb_callback(self, img: Image) -> None:
        """Log RGB image."""
        time = Time.from_msg(img.header.stamp)
        rr.set_time("ros_time", np.datetime64(time.nanoseconds, "ns"))

        cv_img = self.cv_bridge.imgmsg_to_cv2(img, desired_encoding="bgr8")
        rr.log("camera/rgb", rr.Image(cv_img))

    def depth_callback(self, img: Image) -> None:
        """Log depth image."""
        time = Time.from_msg(img.header.stamp)
        rr.set_time("ros_time", np.datetime64(time.nanoseconds, "ns"))

        cv_img = self.cv_bridge.imgmsg_to_cv2(img, desired_encoding="passthrough")
        rr.log("camera/depth", rr.DepthImage(cv_img))


def main(args=None) -> None:

    rclpy.init(args=args)
    node = DataStreamRecorder()

    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
