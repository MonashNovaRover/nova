__package__ = "autonomous"

import numpy as np
import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
from core.msg import RoverPose
import math_utils.transform as transform
from config.runtime_params import tracking_camera_extrinsics, t265_serial, pose_file
from config.ros_config import main_frame, tracking_pose_topic, rover_pose_topic
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

        self.camera_pub = self.create_publisher(Odometry, tracking_pose_topic, 10)        
        self.rover_pose_pub = self.create_publisher(RoverPose, rover_pose_topic, 10)
        self.rover_pose_odom_pub = self.create_publisher(Odometry, "rover/odom", 10)

        # Build config object and request pose data
        self.cfg = rs.config()
        self.cfg.enable_device(serial_number)
        self.cfg.enable_stream(rs.stream.pose)

        answer = input("Load pose from file? (y/n): ")

        self.initial_position = np.array([0.0, 0.0, 0.0])
        self.initial_yaw = 0.0

        if answer and (answer[0] == "y" or answer[0] == "Y"):
            self.load_pose()

        # Start streaming
        self.pipe_profile = self.pipe.start(self.cfg)

    def load_pose(self):
        try:
            pose = np.loadtxt(pose_file).reshape(4)
        except FileNotFoundError as e:
            self.get_logger().warn("Couldn't find file!")
       
        self.initial_position = pose[:3]
        self.initial_yaw = pose[3]

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

        x = -data.translation.z
        y = -data.translation.x
        z = data.translation.y
        
        x -= tracking_camera_extrinsics[0]
        y -= tracking_camera_extrinsics[1]
        z -= tracking_camera_extrinsics[2]

        t265_msg.pose.pose.orientation.x = -data.rotation.z
        t265_msg.pose.pose.orientation.y = -data.rotation.x
        t265_msg.pose.pose.orientation.z = data.rotation.y
        t265_msg.pose.pose.orientation.w = data.rotation.w
        
        if self.initial_yaw != 0:
            pitch, roll, yaw = transform.quat_to_euler(t265_msg)
            qx, qy, qz, qw = transform.euler_to_quat([pitch, roll, yaw + self.initial_yaw])
            
            t265_msg.pose.pose.orientation.x = qx
            t265_msg.pose.pose.orientation.y = qy
            t265_msg.pose.pose.orientation.z = qz
            t265_msg.pose.pose.orientation.w = qw

            # translating local x and y into our frame
            rotation = np.array([[np.cos(self.initial_yaw), -np.sin(self.initial_yaw)], [np.sin(self.initial_yaw), np.cos(self.initial_yaw)]])
            
            pose = np.matmul(rotation, np.array([x, y]).T).T
            x, y = pose[0], pose[1]
 
        t265_msg.pose.pose.position.x = x
        t265_msg.pose.pose.position.y = y
        t265_msg.pose.pose.position.z = z

        # add offset from extrinsics and our initial pose
        t265_msg.pose.pose.position.x += self.initial_position[0]
        t265_msg.pose.pose.position.y += self.initial_position[1]
        t265_msg.pose.pose.position.z += self.initial_position[2]

        return t265_msg

    def get_next_pose(self):
        frames = self.pipe.wait_for_frames()
        pose = frames.get_pose_frame()
        if pose:
            data = pose.get_pose_data()

            t265_msg = self.transform_t265_to_nova(data)
            self.camera_pub.publish(t265_msg)
            rover_msg = RoverPose()
            
            # get rover position as centre of wheel-base
            rover_position = transform.transform_points(t265_msg, np.array([tracking_camera_extrinsics]))[0]

            rover_odom_msg = self.transform_t265_to_nova(data)
            rover_odom_msg.pose.pose.position.x = rover_position[0]
            rover_odom_msg.pose.pose.position.y = rover_position[1]
            rover_odom_msg.pose.pose.position.z = rover_position[2]

            rover_msg.x = rover_position[0]
            rover_msg.y = rover_position[1]
            rover_msg.z = rover_position[2]
            
            # gets euler angles from tracking camera quaternion
            rover_msg.pitch, rover_msg.roll, rover_msg.yaw = transform.quat_to_euler(t265_msg)
            self.rover_pose_pub.publish(rover_msg)
            self.rover_pose_odom_pub.publish(rover_odom_msg)

            with open(pose_file, "w") as f:
                f.write(f"{rover_msg.x}\t{rover_msg.y}\t{rover_msg.z}\t{rover_msg.yaw}")

            sys.stdout.write("\r" + "x: " + str(round(rover_msg.x, 4)).ljust(7)
                             + " | y: " + str(round(rover_msg.y, 4)).ljust(7)
                             + " | z: " + str(round(rover_msg.z, 4)).ljust(7)
                             + " | pitch: " + str(round(rover_msg.pitch, 4)).ljust(7)
                             + " | roll: " + str(round(rover_msg.roll, 4)).ljust(7)
                             + " | yaw: " + str(round(rover_msg.yaw, 4)).ljust(7))
            sys.stdout.flush()


def main():
    rclpy.init()
    camera = TrackingCamera()
    for i in range(1000000):
        camera.get_next_pose()
        time.sleep(0.1)


if __name__ == "__main__":
    main()
