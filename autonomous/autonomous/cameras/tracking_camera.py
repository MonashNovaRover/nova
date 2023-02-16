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
from geometry_msgs.msg import TransformStamped, PoseStamped, Transform
from sensor_msgs.msg import Image, Imu
import cv_bridge

# different systems seem to install the pyrealsense wrapper differently
try:
    import pyrealsense2.pyrealsense2 as rs
except:
    import pyrealsense2 as rs
import sys

"""
Connects to the tracking camera and publishes various transformed pose topics. Runs in a separate thread.
"""

"""
TODO: Get Pose from slam package
TODO: Publish camera and IMU data for slam package
TODO: Publish pose as PoseStamped object over Ros rather than transform
TODO: Modify localisation/pose_converted to listen to tracking camera poses as well as dgps poses
TODO: Do pose conversions and publish tf2 transforms in pose converter instead of tracking camera
"""


class TrackingCamera(Node):
    """
    This object runs in a separate thread and either accepts input directly from the tracking camera, from a ROS node,
    or acts as a ROS-publisher. It maintains an internal state of the most recent tracking camera pose.
    """
    def __init__(self, serial_number=t265_serial):

        super().__init__("T265Node")
        self.param_use_orbslam = self.declare_parameter("use_orbslam", True)

        # Declare RealSense pipeline, encapsulating the actual device and sensors
        self.pipe = rs.pipeline()

        # Publish pose of tracking camera
        self.pub_tracking_cam_pose = self.create_publisher("/T265/Pose", PoseStamped, 10)
        self.pub_left = self.create_publisher("/T265/camera_left_raw/color", Image, 10)
        self.pub_right = self.create_publisher("/T265/camera_right_raw/color", Image, 10)
        self.pub_imu = self.create_publisher("/T265/imu", Imu, 10)

        # Subscriber to slam messages
        self.sub_orbslam_pose = self.create_subscription(PoseStamped, "/slam/pose", self.cb_slam_pose, 10)

        # Build config object and request pose data
        self.cfg = rs.config()
        self.cfg.enable_device(serial_number)
        if self.param_use_orbslam:
            # Enable left and right fisheye streams
            self.cfg.enable_stream(rs.stream.fisheye, 1, rs.format.y8, 30)
            self.cfg.enable_stream(rs.stream.fisheye, 2, rs.format.y8, 30)
            
            # Get IMU data
            self.cfg.enable_stream(rs.stream.accel, rs.format.xyz32f)
            self.cfg.enable_stream(rs.stream.gyro, rs.format.xyz32f)
        else:
            self.cfg.enable_stream(rs.stream.pose)

        self.pose_msg = PoseStamped()

        # Start streaming
        self.pipe_profile = self.pipe.start(self.cfg, self.cb_t265_frame)
        self.create_timer(1./30, self.pub_pose)

    def cb_t265_frame(self, frame: rs.frame):
        # creating ros timestamp for message
        timestamp: float = fs.get_timestamp() * 1e-3
        seconds = int(timestamp)
        nanoseconds = int((timestamp - seconds) * 1e9)
        ros_stamp = Time(seconds=seconds, nanoseconds=nanoseconds)

        if fs:=frame.as_frameset() is not None:

            left_frame = fs.get_fisheye_frame(1)
            right_frame = fs.get_fisheye_frame(1)

            img_left = cv_bridge.CvBridge().cv2_to_imgmsg(np.asanyarray(left_frame.get_data()))
            img_right = cv_bridge.CvBridge().cv2_to_imgmsg(np.asanyarray(right_frame.get_data()))

            img_left.header.frame_id = "T265_left_lens"
            img_right.header.frame_id = "T265_right_lens"


            img_left.header.stamp = ros_stamp
            img_right.header.stamp = ros_stamp

            self.pub_left.publish(img_left)
            self.pub_right.publish(img_right)
        elif motion_frame:=frame.as_motion_frame() is not None:
            if motion_frame.get_profile().stream_name() == "Gyro":

            
    

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
            self.pub_tracking_cam_pose.publish(t265_transform)

    def pub_pose(self):
        self.pub_tracking_cam_pose.publish(self.pose_msg)



def main(args=None):
    rclpy.init()
    camera = TrackingCamera()
    rclpy.spin(camera)
    camera.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()
