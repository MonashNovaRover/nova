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
EDITED:        09/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
# standard imports
import time
import sys
import rclpy
import numpy as np

# autonomous imports
import math_utils.transform as transform
from config.runtime_params import dgps_extrinsics, tracking_camera_extrinsics, pose_pub_rate, minimum_gps_corrections
from config.ros_config import main_frame, camera_pose_topic, rover_pose_topic, auto_goal_topic, auto_goal_gps, rover_odom_topic
from localisation.ekf import Ekf
from localisation.gps_converter import GpsConverter

# ROS imports
from rclpy.node import Node
from nav_msgs.msg import Odometry
from core.msg import RoverPose, WheelData, RoverPoseGPS, AutonomousGoal
from geometry_msgs.msg import PoseWithCovariance
from sensor_msgs.msg import Imu
from rclpy.qos import qos_profile_sensor_data as qos


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
        self.dgps_sub = self.create_subscription(PoseWithCovariance, "/gps_rover/pose_cov", self.dgps_callback, qos)
        self.drive_sub = self.create_subscription(WheelData, "/electronics/wheel_data", self.drive_callback, 10)
        self.goals_sub = self.create_subscription(AutonomousGoal, auto_goal_gps, self.goal_callback, 10)

        # publishers
        self.camera_pub = self.create_publisher(Odometry, camera_pose_topic, 10)

        # coordinate system: 
        #     positive pitch        => up
        #     positive roll         => right (clockwise)
        #     positive yaw (0, 2pi) => counterlockwise
        self.rover_pose_pub = self.create_publisher(RoverPose, rover_pose_topic, 10)
        self.rover_pose_gps_pub = self.create_publisher(RoverPoseGPS, "/electronics/rover_pose_gps", 10)

        self.rover_pose_odom_pub = self.create_publisher(Odometry, rover_odom_topic, 10)
        self.goals_pub = self.create_publisher(AutonomousGoal, auto_goal_topic, 10)

        # timer 
        self.pub_timer = self.create_timer(pose_pub_rate, self.publisher_callback)

        # for maintaining accurate pose and converting between gps
        self.ekf = Ekf()
        self.gps_converter = GpsConverter()

        # state and covariance used for EKF
        self.x = np.zeros((2, 1))
        self.p = np.zeros((2, 2))
        self.u = np.array([[None] * 3]).reshape((3, 1))

        self.rover_pose = RoverPose()
        self.rover_pose_gps = RoverPoseGPS()
        self.odom = Odometry()
        self.odom.header.frame_id = "map"

        self.initial_yaw = 0
        # for publishing

        self.previous_prediction = time.perf_counter()

        # we want to get a lot of gps data before we're confident enough to start publishing pose
        self.num_gps_corrections = 0
        self.received_yaw = False

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

        pitch, roll, yaw = transform.quat_to_euler(imu_odom)
        yaw -= self.gps_converter.magnetic_declination 
        yaw += 0 if yaw > -np.pi else 2 * np.pi

        qx, qy, qz, qw = transform.euler_to_quat((pitch, roll, yaw))
        imu_odom.pose.pose.orientation.x = qx
        imu_odom.pose.pose.orientation.y = qy
        imu_odom.pose.pose.orientation.z = qz
        imu_odom.pose.pose.orientation.w = qw

        return pitch, -roll, yaw, imu_odom

    def imu_callback(self, msg):
        """
        updates msg
        """
        pitch, roll, yaw, imu_odom = self.transform_imu_to_nova(msg)
        self.odom.pose.pose.orientation = imu_odom.pose.pose.orientation

        self.u[1:] = np.array([[pitch], [yaw]])
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
        p_xx, p_xy, p_yx, p_yy = msg.covariance[0], msg.covariance[1], msg.covariance[6], msg.covariance[7]

        """
        if p_xx > 1 or p_yy > 1:
            # high variance -> ignore
            return
        """

        cov = np.array([[p_xx, p_xy], [p_yx, p_yy]])

        # lat and lon
        coord = self.gps_converter.get_local_coord(
                msg.pose.position.x, msg.pose.position.y
                )

        x, y = coord[0], coord[1]

        obs = np.array([x, y]).reshape((2, 1))

        if self.num_gps_corrections == 0:
            self.x = obs
            self.p = cov

        else:
            self.x, self.p = self.ekf.correct_gps(self.x, self.p, obs, cov)

        self.num_gps_corrections += 1

        transformed_dgps_extrinsics = transform.transform_from_quat(self.odom.pose.pose.orientation, np.array(dgps_extrinsics))
        self.odom.pose.pose.position.x = self.x[0, 0] + transformed_dgps_extrinsics[0]
        self.odom.pose.pose.position.y = self.x[1, 0] + transformed_dgps_extrinsics[1]
        self.odom.pose.pose.position.z = 0.0

        cov_odom = [0.] * 36
        cov_odom[0], cov_odom[1] = self.p[0, 0], self.p[1, 0]
        cov_odom[6], cov_odom[7] = self.p[0, 1], self.p[0, 1]
        self.odom.pose.covariance = cov_odom

    def drive_callback(self, msg):
        """
        Takes velocities from all 6 wheels and approximates rover velocity, then propagates a prediction step
        from the ekf
        """
        vel = sum(msg.velocities) / 6.0
        self.u[0, 0] = vel
        if self.num_gps_corrections == 0 or None in self.u:
            return
        if self.num_gps_corrections < minimum_gps_corrections:
            # don't publish until we've collated enough data
            return
        t = time.perf_counter()
        self.x, self.p = self.ekf.predict(self.x, self.p, self.u, t - self.previous_prediction)
        self.previous_prediction = t

    def goal_callback(self, msg):                                               
        local_goal = AutonomousGoal()
                                                                                  
        local_goal.ids = [iD for iD in msg.ids]
        local_goal.position.x, local_goal.position.y = self.gps_converter.get_local_coord(
                  msg.position.x, msg.position.y                                  
                  )                                                               

        self.goals_pub.publish(local_goal)

    def publisher_callback(self):
        """
        On a 5 hz timer, publish all necessary rover poses
        """
        # set z = 0 always
        if self.num_gps_corrections < minimum_gps_corrections:
            # don't publish until we've collated enough data
            return
        self.publish_rover_pose()
        self.odom.header.stamp = self.get_clock().now().to_msg()
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
        rover_msg.pitch, rover_msg.roll, rover_msg.yaw = pitch, -roll, yaw

        self.rover_pose_pub.publish(rover_msg)
        self.print_rover_msg(rover_msg)

        # filling out gps message
        gps_msg.valid = True
        gps_msg.pitch, gps_msg.roll, gps_msg.yaw = pitch, roll, yaw
        # convert x, y to lat, lon
        
        coord = self.gps_converter.get_global_coord(
                rover_msg.x, rover_msg.y
                )
        if coord is None:
            return

        gps_msg.latitude, gps_msg.longitude = coord[0], coord[1]

        self.rover_pose_gps_pub.publish(gps_msg)

    def publish_cam_odom(self):
        """
        Converts self.odom (which is odometry of the rover's wheelbase) into an odometry relative
        to the cameras' positions. This is used to translate points received from the depth camera
        """
        # vector from depth cam to rover centre
        depth_cam_offset = transform.transform_from_quat(self.odom.pose.pose.orientation, np.array(tracking_camera_extrinsics))
        camera_msg = Odometry()
        camera_msg.header.frame_id = "map"
        camera_msg.header.stamp = self.get_clock().now().to_msg()

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
        # with open(pose_file, "w") as f:
        #    f.write(f"{rover_msg.x}\t{rover_msg.y}\t{rover_msg.z}\t{rover_msg.yaw}")


def main():
    rclpy.init()
    converter = PoseConverter()
    rclpy.spin(converter)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
