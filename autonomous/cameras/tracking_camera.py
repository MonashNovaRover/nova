__package__ = "autonomous"

import numpy as np
import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
from core.msg import RoverPose, DriveVel
import math
import math_utils.transform as transform
from config.runtime_params import tracking_camera_extrinsics, t265_serial
from config.ros_config import main_frame, tracking_pose_topic, rover_pose_topic

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
    or acts as a ROS-publisher. It maintains an internal state of the most recent tracking camera pose, can be
    configured to use wheel odometry, and
    """
    def __init__(self, serial_number=t265_serial):

        super().__init__("T265Node")

        # Declare RealSense pipeline, encapsulating the actual device and sensors
        self.pipe = rs.pipeline()

        self.camera_pub = self.create_publisher(Odometry, tracking_pose_topic, 10)        
        self.rover_pose_pub = self.create_publisher(RoverPose, rover_pose_topic, 10)

        # Subscriber for wheel odom data
        # self.wheel_velocity = rs.vector() # holds wheel velocity input
        # self.wheel_subscriber = self.create_subscription(DriveVel, "/autonomous/drive_vel", self.update_wheel_vel, 10)

        # Build config object and request pose data
        self.cfg = rs.config()
        self.cfg.enable_device(serial_number)
        self.cfg.enable_stream(rs.stream.pose)

        self.initial_x = 0.0
        self.initial_y = 0.0

        # Start streaming
        pipe_profile = self.pipe.start(self.cfg) 

        # Initialise wheel odom input
        dev = pipe_profile.get_device()
        # dev.hardware_reset() - could do a hardware reset here?
        # later as mentioned should have a system for detecting if disconnected
        tm2 = dev.as_tm2()
        self.wheel_odometer = None
        if tm2:
            pose_sensor = tm2.first_pose_sensor()
            #self.wheel_odometer = pose_sensor.as_wheel_odometer()
            #self.wheel_odometer.load_wheel_odometery_config(self.toUint8()) # load/configure wheel odometer

    def transform_t265_to_nova(self, data):
        """
        Transform the raw T265 data into a ROS Odom message, with the right handed coorddinate system 
        where 
        up = +z
        left = +y
        forward = +x
        """
        t265_msg = Odometry()

        t265_msg.header.stamp = self.get_clock().now().to_msg()
        t265_msg.header.frame_id = main_frame

        t265_msg.pose.pose.position.x = -data.translation.z
        t265_msg.pose.pose.position.y = -data.translation.x
        t265_msg.pose.pose.position.z = data.translation.y

        t265_msg.pose.pose.orientation.x = -data.rotation.z
        t265_msg.pose.pose.orientation.y = -data.rotation.x
        t265_msg.pose.pose.orientation.z = data.rotation.y
        t265_msg.pose.pose.orientation.w = data.rotation.w
        return t265_msg


    def get_next_pose(self):
        frames = self.pipe.wait_for_frames()
        pose = frames.get_pose_frame()
        if pose:
            data = pose.get_pose_data()
            # calculate position - flip convert to correct x and y conventions
            
            t265_msg = self.transform_t265_to_nova(data)
            self.camera_pub.publish(t265_msg)
            rover_msg = RoverPose()
            
            # get rover position as centre of wheel-base
            rover_position = transform.transform_points(t265_msg, np.array([tracking_camera_extrinsics]))[0]

            rover_msg.x = rover_position[0]
            rover_msg.y = rover_position[1]
            rover_msg.z = rover_position[2]
            
            # gets euler angles from tracking camera quaternion
            rover_msg.pitch, rover_msg.roll, rover_msg.yaw = transform.quat_to_euler(t265_msg)
            self.rover_pose_pub.publish(rover_msg)

            self.send_wheel_odom() # This should be tested if it should go here
            # which is essentially sending the last wheel data recieved OR
            # whether it should fire after the wheel data is recieved under
            # the callback.

            sys.stdout.write("\r" + "x: " + str(round(rover_msg.x, 4)).ljust(7)
                             + " | y: " + str(round(rover_msg.y, 4)).ljust(7)
                             + " | z: " + str(round(rover_msg.z, 4)).ljust(7)
                             + " | pitch: " + str(round(rover_msg.pitch, 4)).ljust(7)
                             + " | roll: " + str(round(rover_msg.roll, 4)).ljust(7)
                             + " | yaw: " + str(round(rover_msg.yaw, 4)).ljust(7))
            sys.stdout.flush()

    def toUint8(self, filename ='cameras/calibration_odometry.json'):
        # calibration to list of uint8
        f = open(filename)
        chars = []
        for line in f:
            for c in line:
                chars.append(ord(c))
        return chars

    def update_wheel_vel(self, msg):
        # Update (currently from drive commands) the wheel velocity
        self.wheel_velocity.x = msg.linear_vel # m/s, must be float
        self.wheel_velocity.y, self.wheel_velocity.z = 0, 0

    def send_wheel_odom(self):
        wo_sensor_id = 0  # indexed from 0, match to order in calibration file
        frame_num = 0  # not used

        #self.wheel_odometer.send_wheel_odometry(wo_sensor_id, frame_num, self.wheel_velocity)

def main():
    rclpy.init()
    camera = TrackingCamera()
    for i in range(1000000):
        camera.get_next_pose()

if __name__ == "__main__":
    main()
