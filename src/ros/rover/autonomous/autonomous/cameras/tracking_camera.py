__package__ = "autonomous"

import numpy as np
import rclpy
from rclpy.node import Node
from rclpy.duration import Duration
from rclpy.time import Time
from nav_msgs.msg import Odometry
import math_utils.transform as transform
from config.runtime_params import tracking_camera_extrinsics, t265_serial, pose_file
from config.ros_config import main_frame, camera_pose_topic, rover_pose_topic
from tf2_ros import TransformBroadcaster, TransformListener, StaticTransformBroadcaster, Buffer
from geometry_msgs.msg import TransformStamped, PoseStamped, Transform
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
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer)

        # Build config object and request pose data
        self.cfg = rs.config()
        self.cfg.enable_device(serial_number)
        self.cfg.enable_stream(rs.stream.pose)

        self.get_initial_transform()

        # Start streaming
        self.pipe_profile = self.pipe.start(self.cfg)

    def get_initial_transform(self, answer):
        """
        If we want to, load initial rover position from file
        """
        # TODO: set in param somewhere?
        answer = input("Load pose from file? (y/n): ")

        initial_transform = TransformStamped()
        initial_transform.header.frame_id = 'map'
        initial_transform.header.stamp = self.get_clock().now().to_msg()
        initial_transform.child_frame_id = 'initial_base_link'
        if answer and (answer[0] == "y" or answer[0] == "Y"):
            initial_transform.transform = self.fill_initial_pose(initial_transform.transform)
        else:
            initial_transform.transform.rotation.w = 1.0

        tf_initial_offset = StaticTransformBroadcaster(self)
        tf_initial_offset.sendTransform(initial_transform)

    def fill_initial_pose(self, transform: Transform):
        """
        Read data from file and fill transform with it
        """
        try:
            pose = np.loadtxt(pose_file).reshape(4)
        except FileNotFoundError as e:
            self.get_logger().warn("Couldn't find file!")
       
        transform.translation.x, transform.translation.y, transform.translation.z = pose[:3]
        # filling in yaw
        transform.rotation.z = np.sin(pose[3] / 2)
        transform.rotation.w = np.cos(pose[3] / 2)
        return transform

    def transform_t265_to_nova(self, data):
        """
        Transform the raw T265 data into a ROS Odom message, with the right handed coorddinate system 
        where 
        up = +z
        left = +y
        forward = +x
        """
        t265_transform = Transform()

        t265_transform.translation.x = -data.translation.z
        t265_transform.translation.y = -data.translation.x
        t265_transform.translation.z = data.translation.y

        t265_transform.rotation.x = -data.rotation.z
        t265_transform.rotation.y = -data.rotation.x
        t265_transform.rotation.z = data.rotation.y
        t265_transform.rotation.w = data.rotation.w

        return t265_transform

    def get_next_pose(self):
        frames = self.pipe.wait_for_frames()
        pose = frames.get_pose_frame()
        if pose is not None:
            data = pose.get_pose_data()

            t265_transform = self.transform_t265_to_nova(data)

            base_link_transform = TransformStamped() 
            base_link_transform.header.stamp = self.get_clock().now().to_msg()
            base_link_transform.header.frame_id = 'initial_base_link'
            base_link_transform.child_frame_id = 'base_link'

            t265_offset = self.tf_buffer.lookup_transform('base_link', 't265', 0).transform
            base_link_transform.transform = transform.offset_transform(transform=t265_transform, offset=t265_offset)
            
            self.tf_base_link.sendTransform(base_link_transform)


def main():
    rclpy.init()
    camera = TrackingCamera()
    for i in range(1000000):
        camera.get_next_pose()
        time.sleep(0.1)


if __name__ == "__main__":
    main()
