#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Remap old ros2 bag timestamps to the 
    current time to integrate with our live
    autonomous system
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: stamp_remapper
TOPICS:
  - subscriber: /T265/pose/old [PoseStamped]
  - subscriber: /depth_camera/d435_1/images/old [Image]
  - subscriber: /depth_camera/d435_1/cloud/old [PointCloud2]
  - publisher: /T265/pose [PoseStamped]
  - publisher: /depth_camera/d435_1/images [Image]
  - publisher: /depth_camera/d435_1/cloud [PointCloud2]
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    autonomous
AUTHOR(S):	Max Tory
CREATION:	25/03/2023
EDITED:		25/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node

# example of how to import a custom message type
from geometry_msgs.msg import PoseStamped
from sensor_msgs.msg import Image, PointCloud2

class BagStampConverter(Node):

    def __init__(self):
        super().__init__("stamp_remapper")
        # way-point publisher publishes a bunch of waypoints at once (hence using the 2D map datatype
        self.pose_subscriber = self.create_subscription(PoseStamped, "/T265/pose/old", self.pose_callback, 10)
        self.cloud_subscriber = self.create_subscription(Image, "/depth_camera/d435_1/image/old", self.image_callback, 10)
        self.image_subscriber = self.create_subscription(PointCloud2, "/depth_camera/d435_1/cloud/old", self.cloud_callback, 10)

        self.pose_publisher = self.create_publisher(PoseStamped, "/T265/pose", 10)
        self.image_publisher = self.create_publisher(Image, "/depth_camera/d435_1/image", 10)
        self.cloud_publisher = self.create_publisher(PointCloud2, "/depth_camera/d435_1/cloud", 10)

    def pose_callback(self, msg: PoseStamped):
        """
        remaps the header stamp and publishes again
        """
        msg.header.stamp = self.get_clock().now().to_msg()
        self.pose_publisher.publish(msg)

    def image_callback(self, msg: Image):
        """
        remaps the header stamp and publishes again
        """
        msg.header.stamp = self.get_clock().now().to_msg()
        self.image_publisher.publish(msg)

    def cloud_callback(self, msg: PointCloud2):
        """
        remaps the header stamp and publishes again
        """
        msg.header.stamp = self.get_clock().now().to_msg()
        self.cloud_publisher.publish(msg)

def main():
    rclpy.init()
    node = BagStampConverter()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
