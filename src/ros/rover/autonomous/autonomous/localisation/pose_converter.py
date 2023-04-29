#!/usr/bin/python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Subscribes to the DGPS and IMU topics and publishes some combined, transformed versions of this information

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: ConverterNode
TOPICS:
  - /imu/euler   [Subscribed]
  - /electronics/gps_data            [Subscribed]
  - /autonomous/goals            [Subscribed]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:  autonomous    
AUTHOR(S): Liam Whittle, Max Tory
CREATION:    06/05/2022
EDITED:        27/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
    - Implement EKF for fun
    - GPS converter takes into account pose offset
    - Use TF2 properly from imu coordinate transformation
"""

# standard imports
import time
import math
import numpy as np

# autonomous imports
import autonomous.math_utils.transform as transform
from autonomous.config.ros_config import auto_goal_topic, auto_goal_gps
from autonomous.localisation.ekf import Ekf
from autonomous.localisation.gps_converter import GpsConverter

# ROS imports
import rclpy
from rclpy.node import Node
from rclpy.time import Time
from rclpy.duration import Duration

from tf2_ros import TransformBroadcaster, StaticTransformBroadcaster, Buffer, TransformListener

from core.msg import RoverPoseGPS, AutonomousGoal, AutonomousGoalArray, WheelData 

from geometry_msgs.msg import TransformStamped, Transform, Vector3Stamped
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
        super().__init__("pose_converter")

        # Ros params
        self.param_do_ekf = self.declare_parameter("do_ekf", False).value
        self.param_gps_frame_id = self.declare_parameter("gps_frame_id", "gps_link").value
        self.param_imu_frame_id = self.declare_parameter("imu_frame_id", "imu_link").value
        self.param_base_link_rate = self.declare_parameter("base_link_pub_rate_hz", 30).value
        self.param_use_euler_angles = self.declare_parameter("use_euler", False).value
        self.param_initial_quat = self.declare_parameter("initial_base_link_quat", [0., 0., 0., 0., 0., 0., 1.]).value
        self.param_initial_euler = self.declare_parameter("initial_base_link_euler", [0., 0., 0., 0., 0., 0.]).value
        
        # subscribers
        self.imu_sub = self.create_subscription(Vector3Stamped, "/imu/euler", self.cb_imu, 10)
        self.gps_sub = self.create_subscription(RoverPoseGPS, "/electronics/gps_data", self.cb_dgps, 10)
        self.goals_sub = self.create_subscription(AutonomousGoal, auto_goal_gps, self.cb_goal, 10)

        # publishers
        self.goals_pub = self.create_publisher(AutonomousGoal, auto_goal_topic, 10)

        # tf2 objects
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, node=self, spin_thread=True)
        self.tf_base_link = TransformBroadcaster(self)
        self.tf_initial_offset = StaticTransformBroadcaster(self)

        # timer 
        self.pub_timer = self.create_timer(1/self.param_base_link_rate, self.cb_publish_transform)

        # state
        self.gps_converter : GpsConverter = GpsConverter()
        self.latest_imu : Vector3Stamped = None
        self.latest_gps_pose : RoverPoseGPS = None
        self.gps_offset : np.ndarray = None
        self.get_initial_transform()

        # for maintaining accurate pose and converting between gps
        if self.param_do_ekf:
            raise NotImplementedError("EKF not integrated yet")

    def fill_initial_pose(self, initial_transform: Transform):
        """
        Read data from params and fill transform with it
        """
        
        if self.param_use_euler_angles:
            try:
                initial_transform.translation.x, initial_transform.translation.y, initial_transform.translation.z = \
                    self.param_initial_euler[0], self.param_initial_euler[1], self.param_initial_euler[2]
                initial_transform.rotation = transform.euler_to_quat(
                    (math.radians(self.param_initial_euler[4]),
                    math.radians(self.param_initial_euler[5]),
                    math.radians(self.param_initial_euler[3]))
                )
                print(f"INITIAL TRANSFORM: {initial_transform}")
            except IndexError as e:
                self.get_logger().error(f"Incorrect euler angle layout in parameter: {e}")
            except Exception as e:
                self.get_logger().error(f"Failed to read parameter initial pose {e}")
        else:
            try:
                initial_transform.translation.x, initial_transform.translation.y, initial_transform.translation.z = \
                    self.param_initial_quat[0], self.param_initial_quat[1], self.param_initial_quat[2]
                initial_transform.rotation.x, initial_transform.rotation.y, initial_transform.rotation.z, initial_transform.rotation.w = \
                    self.param_initial_quat[3], self.param_initial_quat[4], self.param_initial_quat[5], self.param_initial_quat[6]
            except IndexError as e:
                self.get_logger().error(f"Incorrect quaternion layout in parameter: {e}")
            except Exception as e:
                self.get_logger().error(f"Failed to read parameter initial pose {e}")

        return initial_transform

    def get_initial_transform(self):
        """
        If we want to, load initial transform from params
        """
        initial_transform = TransformStamped()
        initial_transform.header.frame_id = 'map'
        initial_transform.header.stamp = self.get_clock().now().to_msg()
        initial_transform.child_frame_id = 'initial_base_link'
        # TODO : Allow initial offsets for some reason
        # initial_transform.transform = self.fill_initial_pose(initial_transform.transform)

        initial_transform.transform.rotation.w = 1.0

        self.tf_initial_offset.sendTransform(initial_transform)

        base_link_transform = TransformStamped()
        base_link_transform.header.frame_id = 'initial_base_link'
        base_link_transform.header.stamp = self.get_clock().now().to_msg()
        base_link_transform.child_frame_id = 'base_link'

        base_link_transform.transform.rotation.w = 1.0
        base_link_transform.transform.rotation.x = 0.0
        base_link_transform.transform.rotation.y = 0.0
        base_link_transform.transform.rotation.z = 0.0
        self.tf_base_link.sendTransform(base_link_transform)

        self.get_logger().info("Waiting for GPS offset transform...")
        while not self.tf_buffer.can_transform("base_link", "gps_link", Time()):
            time.sleep(0.1)

        self.get_logger().info("Received GPS offset transform")
        gps_tf = self.tf_buffer.lookup_transform("base_link", "gps_link", Time()).transform
        r, p, y = transform.quat_to_euler(gps_tf.rotation)
        self.gps_offset = np.array([0., 0., y])

    def transform_imu_to_nova(self):
        """
        Transform the fused imu data into a ROS Odom message, with the right handed coordinate system
        where
        up = +z
        left = +y
        forward = +x
        TODO: set IMU transform in URDF, and we can use tf2 to transform properly. Note that there are serious complications
        Caused by IMUs having absolute pitch and roll but relative yaw, so this is non-trivial
        """
        imu_roll, imu_pitch = math.radians(self.latest_imu.vector.x), math.radians(self.latest_imu.vector.y)
        return np.array([imu_roll, imu_pitch, 0.])

    def calculate_transform(self) -> TransformStamped:
        """
        Calculate the transform between the intial_base_link and the base_link frame
        Converts the latest IMU euler angles and latest dgps latitude, longitude and yaw into a combined
        TransformStamped message, then returns that
        """
        # Take pitch and roll from imu and heading from gps
        imu_eulers = self.transform_imu_to_nova()
        # GPS heading is positive in the clockwise direction, but the right hand rule dictates for us that positive yaw is counter-clockwise
        gps_eulers = np.array([0., 0., -math.radians(self.latest_gps_pose.yaw)])
        eulers = imu_eulers + gps_eulers - self.gps_offset
        quat = transform.euler_to_quat(eulers)
        gps_x, gps_y = self.gps_converter.get_local_coord(self.latest_gps_pose.latitude, self.latest_gps_pose.longitude)

        tf_msg = TransformStamped()
        tf_msg.header.frame_id = 'initial_base_link'
        tf_msg.header.stamp = self.get_clock().now().to_msg()
        tf_msg.child_frame_id = 'base_link'

        tf_msg.transform.translation.x = gps_x
        tf_msg.transform.translation.y = gps_y

        tf_msg.transform.rotation = quat
        
        return tf_msg

    def cb_imu(self, msg: Vector3Stamped):
        """
        Updates locally stored message
        """
        self.latest_imu = msg

    def cb_dgps(self, msg : RoverPoseGPS):
        """
        Updates locally stored message
        """
        if msg.valid:
            self.latest_gps_pose = msg

    def cb_goal(self, msg : AutonomousGoalArray):                                               
        """
        Takes GPS coordinate goals, transforms them one-by-one into x, y coordinates and publishes them
        """
        self.get_logger().debug("received goal: {msg}")
        transformed_goals = AutonomousGoalArray()
        transformed_goals.header = msg.header

        goal : AutonomousGoal
        for goal in msg.goals:
            goal.position.x, goal.position.y = self.gps_converter.get_local_coord(
                    goal.position.x, goal.position.y
            )
            transformed_goals.goals.append(goal)

        self.goals_pub.publish(transformed_goals)

    def cb_publish_transform(self):
        """
        On a 5 hz timer, publish all necessary rover poses
        """
        if self.latest_gps_pose is None or self.latest_imu is None:
            self.get_logger().info("Waiting for imu and dgps to come online...", once=True)
        else:
            self.get_logger().info("Received imu and dgps data, publishing transforms...", once=True)
            tf = self.calculate_transform()
            self.tf_base_link.sendTransform(tf)


def main():
    rclpy.init()
    converter = PoseConverter()
    rclpy.spin(converter)
    converter.destroy_node()
    rclpy.shutdown()
    
if __name__ == "__main__":
    main()
