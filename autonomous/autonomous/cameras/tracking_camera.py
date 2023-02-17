#!/usr/bin/python3
__package__ = "autonomous"
import numpy as np
import rclpy
from rclpy.node import Node
from rclpy.time import Time
from rclpy.duration import Duration
import autonomous.math_utils.transform as transform
from autonomous.config.runtime_params import t265_serial, pose_file
from tf2_ros import TransformBroadcaster, TransformListener, StaticTransformBroadcaster, Buffer
from geometry_msgs.msg import TransformStamped, PoseStamped, Pose
import time

# different systems seem to install the pyrealsense wrapper differently
try:
    import pyrealsense2.pyrealsense2 as rs
except:
    import pyrealsense2 as rs
import sys

"""
Connects to the tracking camera and publishes various transformed pose topics. Runs in a separate thread.
"""


class TrackingCamera(Node):
    """
    This object runs in a separate thread and either accepts input directly from the tracking camera, from a ROS node,
    or acts as a ROS-publisher. It maintains an internal state of the most recent tracking camera pose.
    """
    def __init__(self, serial_number=t265_serial):
        super().__init__("T265Node")
        # Declare RealSense pipeline, encapsulating the actual device and sensors
        self.pipe = rs.pipeline()

        self.tf_base_link = TransformBroadcaster(self)

        # Build config object and request pose data
        self.cfg = rs.config()
        self.cfg.enable_device(serial_number)
        self.cfg.enable_stream(rs.stream.pose)

        self.pub_pose = self.create_publisher(PoseStamped, "T265/pose", 10)

        # Start streaming
        self.pipe_profile = self.pipe.start(self.cfg)
        self.create_timer(1./30, self.get_next_pose)

    def transform_t265_to_nova(self, data):
        """
        Transform the raw T265 data into a ROS Odom message, with the right handed coorddinate system
        where
        up = +z
        left = +y
        forward = +x
        """
        t265_pose = Pose()

        t265_pose.position.x = -data.translation.z
        t265_pose.position.y = -data.translation.x
        t265_pose.position.z = data.translation.y

        t265_pose.orientation.x = -data.rotation.z
        t265_pose.orientation.y = -data.rotation.x
        t265_pose.orientation.z = data.rotation.y
        t265_pose.orientation.w = data.rotation.w

        return t265_pose

    def get_next_pose(self):
        frames = self.pipe.wait_for_frames()
        pose = frames.get_pose_frame()
        if pose is not None:
            data = pose.get_pose_data()

            t265_pose: Pose = self.transform_t265_to_nova(data)

            t265_pose_stamped = PoseStamped()
            t265_pose_stamped.header.stamp = self.get_clock().now().to_msg()
            t265_pose_stamped.header.frame_id = 'map'

            t265_pose_stamped.pose = t265_pose
            self.pub_pose.publish(t265_pose_stamped)



def main(args=None):
    rclpy.init()
    camera = TrackingCamera()
    rclpy.spin(camera)
    camera.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()
