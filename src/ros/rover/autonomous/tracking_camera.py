import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
from core.msg import RoverPose

import math
try:
    import pyrealsense2.pyrealsense2 as rs
except:
    import pyrealsense2 as rs
import sys

"""
Connects to the tracking camera and publishes various transformed pose topics. Runs in a seperate thread.
"""

class TrackingCamera(Node):
    """
    This object runs in a separate thread and either accepts input directly from the tracking camera, from a ROS node,
    or acts as a ROS-publisher. It maintains an internal state of the most recent tracking camera pose, can be
    configured to use wheel odometry, and
    """
    def __init__(self, mode="python", publish_to_ros=True, serial_number='952322110473'):

        super().__init__("T265Node")

        # Declare RealSense pipeline, encapsulating the actual device and sensors
        self.pipe = rs.pipeline()

        self.camera_pub = self.create_publisher(Odometry, "/t265/odom/sample", 10)
        self.rover_pose_pub = self.create_publisher(Odometry, "/rover/pose", 10)

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

            msg = Odometry()
            msg.header.stamp = self.get_clock().now().to_msg()
            msg.header.frame_id = "map"

            # update translation
            # todo: confirm these transforms
            msg.pose.pose.position.x = -data.translation.z
            msg.pose.pose.position.y = -data.translation.x
            msg.pose.pose.position.z = data.translation.y

            msg.pose.pose.orientation.x = -data.rotation.z
            msg.pose.pose.orientation.y = -data.rotation.x
            msg.pose.pose.orientation.z = data.rotation.y
            msg.pose.pose.orientation.w = data.rotation.w

            self.camera_pub.publish(msg)
            self.publish_auto_pose(data)

    def publish_auto_pose(self, data):
            msg = RoverPose()
            # calculate position - flip convert to correct x and y conventions
            msg.x = -data.translation.z + self.initial_y
            msg.y = data.translation.x + self.initial_x

            # calculate yaw - convert from quaternion to euler
            qx = data.rotation.x
            qy = data.rotation.z
            qz = data.rotation.y
            qw = data.rotation.w

            # msg.yaw = euler_from_quaternion([q_x, q_y, q_z, q_w])[1]
            yaw = -math.atan2(2.0*(qx*qy + qw*qz), qw*qw + qx*qx - qy*qy - qz*qz)
            yaw = (yaw if yaw > 0 else 2.0 * math.pi + yaw) + 0
            yaw += self.initial_yaw
            yaw = yaw if yaw <= math.pi * 2 else yaw - math.pi * 2

            msg.yaw = yaw

            # calculate velocity - use pythagoras
            x_vel = data.velocity.x
            z_vel = data.velocity.z
            msg.velocity = math.sqrt(x_vel ** 2 + z_vel ** 2)

            # calculate angular velocity - extracting correct axis

            msg.angular_velocity = -data.angular_velocity.y

            sys.stdout.write("\r" + "x: " + str(round(msg.x, 4)).ljust(7) + " | y: " + str(round(msg.x, 4)).ljust(7) + " | yaw: " + str(round(msg.yaw, 4)).ljust(7))
            sys.stdout.flush()
            # todo: create publisher
            # publisher.publish(msg)


def main():
    rclpy.init()
    camera = TrackingCamera()
    for i in range(1000000):
        camera.get_next_pose()


if __name__ == "__main__":
    main()
