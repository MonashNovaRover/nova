#!/usr/bin/python3
__package__ = "autonomous"
import numpy as np
import rclpy
from rclpy.node import Node
from autonomous.config.runtime_params import t265_serial
from geometry_msgs.msg import Pose, PoseStamped
import logging

from core.msg import WheelOdometry
from geometry_msgs.msg import Vector3

# different systems seem to install the pyrealsense wrapper differently
try:
    import pyrealsense2.pyrealsense2 as rs
except:
    import pyrealsense2 as rs
from pyrealsense2 import vector as Vector

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
        self.get_logger().set_level(logging.DEBUG)
        self.pipe = rs.pipeline()

        self.param_do_odometry = self.declare_parameter("do_odometry", False).value
        self.get_logger().debug(f"do odometry? {self.param_do_odometry}")
        self.wheel_odometer = None

        # Build config object and request pose data
        self.cfg = rs.config()
        self.cfg.enable_device(serial_number)
        self.cfg.enable_stream(rs.stream.pose)

        self.pub_pose = self.create_publisher(PoseStamped, "T265/pose", 10)

        if self.param_do_odometry:
            self.setup_odom()
            self.frame_num = 0
            self.sub_odom = self.create_subscription(WheelOdometry, "/localisation/wheel_odom", self.cb_odom, 10)

        # Start streaming
        self.pipe_profile = self.pipe.start(self.cfg)
        self.create_timer(1./30, self.get_next_pose)
        self.get_logger().info("Tracking camera node up!")

    def setup_odom(self):
        """
        What is happening here?
        Excellent question
        """
        # profile for the current open pipeline with the t265 device?
        profile = self.cfg.resolve(self.pipe)
        # The t265 device?
        dev = profile.get_device()
        # interface for T2XX devices, such as T265
        tm2 = dev.as_tm2()
        # gets pose sensor?
        pose_sensor = tm2.first_pose_sensor()
        # Re-cast to a wheel odometer
        self.wheel_odometer = pose_sensor.as_wheel_odometer()
        # load extrinsics from calibration json as char array
        self.wheel_odometer.load_wheel_odometery_config(self.get_calib_chars()) 

    def get_calib_chars(self):
        """
        Gets the calibration json file from the package and returns it as a char array
        """
        import pkg_resources
        calib_file = pkg_resources.resource_filename('autonomous', 'cameras/calibration_odometry.json')
        chars = []
        with open(calib_file, 'r') as f:
            for line in f:
                for c in line:
                    chars.append(ord(c))
        return chars
    
    def ros_to_rs_vector(self, ros_vector: Vector3):
        """
        Convert ros2 Vector3 type to pyrealsense2 vector type
        """
        out_vec = Vector()
        out_vec.x = ros_vector.x
        out_vec.y = ros_vector.y
        out_vec.z = ros_vector.z
        return out_vec

    def cb_odom(self, msg : WheelOdometry):
        """
        Callback for wheel odometry messages. Updates the wheel odometry object with the new data.
        """
        self.get_logger().debug(f"Sending wheel odometry: {msg} \nfor frame {self.frame_num}")
        self.wheel_odometer.send_wheel_odometry(0, self.frame_num, self.ros_to_rs_vector(msg.left_wheel_vel))
        self.wheel_odometer.send_wheel_odometry(1, self.frame_num, self.ros_to_rs_vector(msg.right_wheel_vel))
        self.frame_num += 1

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
