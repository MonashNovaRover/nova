#!/usr/bin/python3
__package__ = "autonomous"
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Subscribes to the DGPS and IMU topics and publishes some combined, transformed versions of this information

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: ConverterNode
TOPICS:
  - /imu/euler   [Subscribed]
  - /            [Subscribed]
  - /imu/euler   [Publisher]
  - /            [Published]
  - /            [Published]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:  autonomous    
AUTHOR(S): Liam Whittle, Max Tory
CREATION:    06/05/2022
EDITED:        08/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
# standard imports
import time
import sys
import rclpy
import numpy as np

# autonomous imports
import math_utils.transform as transform
from config.runtime_params import dgps_extrinsics, tracking_camera_extrinsics, pose_file, minimum_gps_corrections
from config.ros_config import main_frame, tracking_pose_topic, rover_pose_topic, gps_to_xyz_topic, xyz_to_gps_topic
from localisation.ekf import Ekf

# ROS imports
from rclpy.node import Node
from nav_msgs.msg import Odometry
from core.msg import RoverPose, Point2D, WheelData, RoverPoseGPS
from core.srv import PointTransform
from geometry_msgs.msg import PoseWithCovariance
from sensor_msgs.msg import Imu


class PoseConverter(Node):
    """
    ROS2 Node to listen to:
        - DGPS
        - IMU
    And publish 4 different transformed coordinate frames
    Uses ekf for accurate pose and for fun
    """

    def __init__(self):
        super().__init__("ConverterNode")
        # subscribers
        self.imu_sub = self.create_subscription(Imu, "/imu/data", self.imu_callback, 10)
        # TODO: Find the LEIGH topic
        self.dgps_sub = self.create_subscription(PoseWithCovariance, "/topic/Leigh", self.dgps_callback, 10)
        self.drive_sub = self.create_subscription(WheelData, "/electronics/wheel_data", self.drive_callback, 10)

        # publishers
        self.camera_pub = self.create_publisher(Odometry, tracking_pose_topic, 10)
        self.rover_pose_pub = self.create_publisher(RoverPose, rover_pose_topic, 10)
        self.rover_pose_gps_pub = self.create_publisher(RoverPoseGPS, "/electronics/rover_pose_gps", 10)
        self.rover_pose_odom_pub = self.create_publisher(Odometry, "/rover/odom", 10)

        # clients
        self.gps_to_xyz_client = self.create_client(PointTransform, gps_to_xyz_topic)
        self.xyz_to_gps_client = self.create_client(PointTransform, xyz_to_gps_topic)

        # for maintaining accurate pose
        self.ekf = Ekf()

        # state and covariance used for EKF
        self.x = np.array([None]*5)
        self.p = np.eye(5)
        self.u = np.zeros((3, 1))   # drive inputs

        self.roll = 0

        self.initial_yaw = 0
        # for publishing
        self.odom = None

        self.previous_prediction = time.perf_counter()

        # we want to get a lot of gps data before we're confident enough to start publishing pose
        self.num_gps_corrections = 0

    def transform_imu_to_nova(self, imu_msg):
        """
        Transform the fused imu data into a ROS Odom message, with the right handed coordinate system
        where
        up = +z
        left = +y
        forward = +x
        """
        imu_odom = Odometry()

        imu_odom.header.stamp = self.get_clock().now().to_msg()
        imu_odom.header.frame_id = main_frame

        imu_odom.pose.pose.orientation.x = imu_msg.orientation.x
        imu_odom.pose.pose.orientation.y = imu_msg.orientation.y
        imu_odom.pose.pose.orientation.z = imu_msg.orientation.z
        imu_odom.pose.pose.orientation.w = imu_msg.orientation.w

        pitch, roll, yaw = transform.quat_to_euler(imu_msg)
        qx, qy, qz, qw = transform.euler_to_quat([pitch, roll, -yaw + self.initial_yaw])

        imu_odom.pose.pose.orientation.x = qx
        imu_odom.pose.pose.orientation.y = qy
        imu_odom.pose.pose.orientation.z = qz
        imu_odom.pose.pose.orientation.w = qw

        return pitch, -yaw, imu_odom

    def imu_callback(self, msg):
        """
        updates msg
        """
        pitch, yaw, imu_odom = self.transform_imu_to_nova(msg)
        self.odom.orientation = imu_odom.pose.pose.orientation

        self.x[2:4] = np.array([[pitch], [yaw]])
        # set rate of change of pitch and yaw
        self.u[1:3] = np.array([[msg.angular_velocity.x], [msg.angular_velocity.z]])
        # uncomment if we work out how to do covariance of imu measurements
        """
        p_xx, p_xy, p_yx, p_yy = msg.orientation_covariance[0], msg.orientation_covariance[1],\
                                    msg.orientation_covariance[4], msg.orientation_covariance[5]

        x = np.array([[pitch], [yaw]])
        P = np.array([[p_xx, p_xy], [p_yx, p_yy]])

        self.x, self.p = self.ekf.correct_imu(self.x, self.p, x, P)
        """

    def dgps_callback(self, msg):
        """
        Gets data from a gps message, converts it to local coordinates, and updates
        the ekf model with its covariance (if the variance of lat and long is not too high)
        """
        # getting covariance values
        p_xx, p_xy, p_yx, p_yy = msg.covariance[0], msg.covariance[1], msg.covariance[1], msg.covariance[7]

        if p_xx > 1 or p_yy > 1:
            # high covariance -> ignore
            return

        self.num_gps_corrections += 1

        cov = np.array([[p_xx, p_xy], [p_yx, p_yy]])

        coord = Point2D()
        # lat and lon
        coord.x, coord.y = msg.pose.position.x, msg.pose.position.y
        req = PointTransform.Request()
        req.point = coord

        # ask service for xyz coordinates in local frame
        future = self.gps_to_xyz_client.call_async(req)
        rclpy.spin_until_future_complete(self, future)
        res = future.result()
        x, y = res.transformed.x, res.transformed.y

        obs = np.array([[x], [y], [0]]) + transform.transform_points(self.odom, dgps_extrinsics)

        if self.x[0] is None:
            self.x[:2] = obs[:2]
            self.p[:2, :2] = cov

        self.x, self.p = self.ekf.correct_gps(self.x, self.p, obs[:2], cov)
        self.odom.pose.pose.position.x = self.x[0, 0]
        self.odom.pose.pose.position.y = self.x[1, 0]
        self.odom.pose.pose.position.z = 0.0

    def drive_callback(self, msg):
        """
        Takes velocities from all 6 wheels and approximates rover velocity, then propagates a prediction step
        from the ekf
        """
        vel = sum(msg.velocities) / 6.0
        self.u[0, :] = vel
        t = time.perf_counter()
        self.x, self.p = self.ekf.predict(self.x, self.p, self.u, t - self.previous_prediction)
        self.previous_prediction = t


    def publisher_timer(self):
        """
        On a 5 hz timer, publish all necessary rover poses
        """
        # set z = 0 always
        if self.num_gps_corrections < minimum_gps_corrections:
            # don't publish until we've collated enough data
            return
        self.publish_rover_pose()
        self.rover_pose_odom_pub.publish(self.odom)
        self.publish_cam_odom()

    def publish_rover_pose(self):
        """
        Publish the rover's pose as an x, y, z, pitch, roll, yaw (no quaternions)
        Also publishes gps coordinates based on the x and y by calling the xyz_to_gps service
        """
        gps_msg = RoverPoseGPS()
        rover_msg = RoverPose()

        # get rover position as centre of wheel-base
        rover_msg.x = self.odom.pose.pose.position.x
        rover_msg.y = self.odom.pose.pose.position.y
        rover_msg.z = self.odom.pose.pose.position.z

        # gets euler angles from imu quaternion
        pitch, roll, yaw = transform.quat_to_euler(self.odom)
        rover_msg.pitch, rover_msg.roll, rover_msg.yaw = pitch, roll, yaw

        self.rover_pose_pub.publish(rover_msg)
        self.print_rover_msg(rover_msg)

        # filling out gps message
        gps_msg.valid = True
        gps_msg.pitch, gps_msg.roll, gps_msg.yaw = pitch, roll, yaw
        # convert x, y to lat, lon
        req = PointTransform.Request()
        req.point.x, req.point.y = rover_msg.x, rover_msg.y
        future = self.xyz_to_gps_client.call_async(req)
        rclpy.spin_until_future_complete(self, future)

        res = future.result()

        gps_msg.latitude, gps_msg.longitude = res.transformed.x, res.transformed.y

        self.rover_pose_gps_pub.publish(gps_msg)

    def publish_cam_odom(self):
        """
        Converts self.odom (which is odometry of the rover's wheelbase) into an odometry relative
        to the cameras' positions. This is used to translate points received from the depth camera
        """
        # vector from depth cam to rover centre
        depth_cam_offset = transform.transform_points(self.odom, tracking_camera_extrinsics)
        camera_msg = Odometry()

        camera_msg.pose.pose.orientation.x = self.odom.pose.pose.orientation.x
        camera_msg.pose.pose.orientation.y = self.odom.pose.pose.orientation.y
        camera_msg.pose.pose.orientation.z = self.odom.pose.pose.orientation.z
        camera_msg.pose.pose.orientation.w = self.odom.pose.pose.orientation.w

        camera_msg.pose.pose.position.x = self.odom.pose.pose.position.x - depth_cam_offset[0]
        camera_msg.pose.pose.position.y = self.odom.pose.pose.position.y - depth_cam_offset[1]
        camera_msg.pose.pose.position.z = self.odom.pose.pose.position.z - depth_cam_offset[2]

        self.camera_pub.publish(camera_msg)

    def print_rover_msg(self, rover_msg):
        # write to system
        sys.stdout.write("\r" + "x: " + str(round(rover_msg.x, 4)).ljust(7)
                         + " | y: " + str(round(rover_msg.y, 4)).ljust(7)
                         + " | z: " + str(round(rover_msg.z, 4)).ljust(7)
                         + " | pitch: " + str(round(rover_msg.pitch, 4)).ljust(7)
                         + " | roll: " + str(round(rover_msg.roll, 4)).ljust(7)
                         + " | yaw: " + str(round(rover_msg.yaw, 4)).ljust(7))
        sys.stdout.flush()

        # I guess we still do this?
        with open(pose_file, "w") as f:
            f.write(f"{rover_msg.x}\t{rover_msg.y}\t{rover_msg.z}\t{rover_msg.yaw}")


def main():
    rclpy.init()
    converter = PoseConverter()
    rclpy.spin(converter)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
