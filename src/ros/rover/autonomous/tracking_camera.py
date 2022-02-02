import numpy as np
import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
from core.msg import RoverPose
import math
import transform
from config.ros_config import tracking_camera_extrinsics
from config.ros_config import main_frame
from config.ros_config import tracking_pose_topic
from config.ros_config import rover_pose_topic

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
    def __init__(self, serial_number='952322110473'):

        super().__init__("T265Node")

        # Declare RealSense pipeline, encapsulating the actual device and sensors
        self.pipe = rs.pipeline()

        self.camera_pub = self.create_publisher(Odometry, tracking_pose_topic, 10)
        self.rover_pose_pub = self.create_publisher(RoverPose, rover_pose_topic, 10)

        # Build config object and request pose data
        self.cfg = rs.config()
        self.cfg.enable_device(serial_number)
        self.cfg.enable_stream(rs.stream.pose)

        self.initial_x = 0.0
        self.initial_y = 0.0
        self.initial_yaw = 5.0 / 4.0 * math.pi

        self.pipe.start(self.cfg)

    def get_next_pose(self):
        frames = self.pipe.wait_for_frames()
        pose = frames.get_pose_frame()
        if pose:
            data = pose.get_pose_data()
            # calculate position - flip convert to correct x and y conventions

            t265_msg = Odometry()
            rover_msg = RoverPose()

            t265_msg.header.stamp = self.get_clock().now().to_msg()
            t265_msg.header.frame_id = main_frame

            t265_msg.pose.pose.position.x = -data.translation.z
            t265_msg.pose.pose.position.y = -data.translation.x
            t265_msg.pose.pose.position.z = data.translation.y

            t265_msg.pose.pose.orientation.x = -data.rotation.z
            t265_msg.pose.pose.orientation.y = -data.rotation.x
            t265_msg.pose.pose.orientation.z = data.rotation.y
            t265_msg.pose.pose.orientation.w = data.rotation.w

            self.camera_pub.publish(t265_msg)

            # get rover position as centre of wheel-base
            rover_position = transform.transform_points(t265_msg, np.array([tracking_camera_extrinsics]))[0]

            rover_msg.x = rover_position[0]
            rover_msg.y = rover_position[1]
            rover_msg.z = rover_position[2]

            qx = data.rotation.x
            qy = data.rotation.y
            qz = data.rotation.z
            qw = data.rotation.w

            # msg.yaw = euler_from_quaternion([q_x, q_y, q_z, q_w])[1]
            yaw = -math.atan2(2.0*(qx*qy + qw*qz), qw*qw + qx*qx - qy*qy - qz*qz)
            yaw = (yaw if yaw > 0 else 2.0 * math.pi + yaw) + 0
            yaw += self.initial_yaw
            yaw = yaw if yaw <= math.pi * 2 else yaw - math.pi * 2

            rover_msg.yaw = yaw

            self.rover_pose_pub.publish(rover_msg)

            sys.stdout.write("\r" + "x: " + str(round(rover_msg.x, 4)).ljust(7)
                             + " | y: " + str(round(rover_msg.x, 4)).ljust(7)
                             + " | yaw: " + str(round(rover_msg.yaw, 4)).ljust(7))
            sys.stdout.flush()


def main():
    rclpy.init()
    camera = TrackingCamera()
    for i in range(1000000):
        camera.get_next_pose()


if __name__ == "__main__":
    main()
