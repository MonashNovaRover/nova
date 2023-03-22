#!/usr/bin/python3
__package__ = "autonomous"
import numpy as np
import rclpy
from rclpy.node import Node
from autonomous.config.runtime_params import t265_serial
from geometry_msgs.msg import Pose, PoseStamped
import logging

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
        self.get_logger().set_level(logging.INFO)
        self.pipe = rs.pipeline()

        # Build config object and request pose data
        self.cfg = rs.config()
        self.cfg.enable_device(serial_number)
        self.cfg.enable_stream(rs.stream.pose)

        self.pub_pose = self.create_publisher(PoseStamped, "T265/pose", 10)

        # Start streaming
        self.pipe_profile = self.pipe.start(self.cfg)
        self.create_timer(1./30, self.get_next_pose)
        self.get_logger().info("Tracking camera node up!")

    def get_next_pose(self):
        frames = self.pipe.wait_for_frames()
        pose = frames.get_pose_frame()
        if pose is not None:
            data = pose.get_pose_data()
            pose = Pose()
            pose.position.x = data.translation.x
            pose.position.y = data.translation.y
            pose.position.z = data.translation.z
            pose.orientation.x = data.rotation.x
            pose.orientation.y = data.rotation.y
            pose.orientation.z = data.rotation.z
            pose.orientation.w = data.rotation.w

            self.get_logger().debug(f"Tracking camera pose: {pose}", throttle_duration_sec=1)

            t265_pose_stamped = PoseStamped()
            t265_pose_stamped.header.stamp = self.get_clock().now().to_msg()
            t265_pose_stamped.header.frame_id = 't265'

            t265_pose_stamped.pose = pose
            self.pub_pose.publish(t265_pose_stamped)



def main(args=None):
    rclpy.init()
    camera = TrackingCamera()
    rclpy.spin(camera)
    camera.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()
