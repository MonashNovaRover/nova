#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Listen to /control/autonomous_commands
    and simulate driving of rover for off-rover
    testing. Publishes tf2 transform and
    PivotWheelData to emulate sensor readings
    and control feedback as if we were really
    autonomously driving the rover.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: drive_sim_node
TOPICS:
  - subscriber: 
        /control/autonomous_commands [DriveInput]
  - publisher: 
        /tf [TransformStamped]
  - publisher: 
        /control/pivot_wheel [PivotWheelData]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        control
AUTHOR(S):	Max Tory
CREATION:	14/05/2023
EDITED:		14/05/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Fix mapping transforms and nodes generally so
    we can get all transforms even with no depth
    cam
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
# ros imports
import rclpy
from rclpy.node import Node
from rclpy.duration import Duration
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from tf2_ros import TransformBroadcaster, StaticTransformBroadcaster

# msg imports
from core.msg import DriveInput, PivotWheelData
from geometry_msgs.msg import Transform, TransformStamped

# nova import
from autonomous.math_utils import transform

# python imports 
from typing import Tuple
import numpy as np

MAX_VEL_M_S = 6

CHASSIS_WIDTH = 0.7
CHASSIS_LENGTH = 0.84

def calculate_max_wheel_radius(radius: float) -> float:
    """
    Calculate the maximum distance of a wheel from the centre of a turn.
    This limits how fast we can turn
    """
    x_dist = CHASSIS_LENGTH / 2
    y_dist_l = abs(radius + CHASSIS_WIDTH / 2)
    y_dist_r = abs(radius - CHASSIS_WIDTH / 2)
    y_dist = max(y_dist_l, y_dist_r)
    return (x_dist ** 2 + y_dist ** 2) ** 0.5


def delta_pose_from_dist_radius(signed_dist: float, radius: float, turn_dir: int) -> Tuple[float, float, float]:
    """
    Take a signed distance and radius, and assuming we have moved that distance around
    the circumference of a circle, return the dx, dy, and dtheta associated
    with this transformation. Works for any units so long as the dist and radius are given
    in the same units
    :returns dx - distance in forward direction
    :returns dy - distance in left direction
    :returns dtheta - radians
    """
    if radius == float('inf'):
        # Straight line
        return signed_dist, 0.0, 0.0
    dist_sign = np.sign(signed_dist)
    dist_abs = np.abs(signed_dist)

    rad_sign = turn_dir
    rad_abs = radius

    rad_wheels = calculate_max_wheel_radius(rad_abs)

    # radians are cool
    dtheta_abs = dist_abs / rad_wheels
    if signed_dist > np.pi * radius or radius == 0:
        # Very tight turn
        return 0.0, 0.0, -1 * dist_sign * rad_sign * dtheta_abs
    dy_abs = rad_abs * (1 - np.cos(dtheta_abs))
    dx_abs = rad_abs * np.sin(dtheta_abs)

    dtheta = -1 * dist_sign * rad_sign * dtheta_abs
    dy = -1 * rad_sign * dy_abs
    dx = dist_sign * dx_abs

    return dx, dy, dtheta


class DriveSimNode(Node):

    def __init__(self):
        super().__init__("drive_sim_node")

        # Drive commands subscriber QoS
        deadline = Duration(nanoseconds=2e8)        
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        # Subscribe to Drive Inputs from autonomous
        self.sub_manual = self.create_subscription(DriveInput, "/control/drive_inputs", self.cb_drive_sub, self.qos)

        self.tf_broadcaster = TransformBroadcaster(self)
        self.static_tf_broadcaster = StaticTransformBroadcaster(self)
        self.pub_pivot_wheel = self.create_publisher(PivotWheelData, "/control/pivot_wheel", 10)

        self.state_current_transform : TransformStamped = None
        self.state_current_drive_input : DriveInput = None
        self.state_current_pivot_wheel : PivotWheelData = PivotWheelData()

        self.initialise_transform()

        self.pub_rate_hz = 0.05  # run the timer 10 times per second
        self.timer_pub_tf = self.create_timer(self.pub_rate_hz, self.cb_tf_timer)

    def cb_drive_sub(self, msg: DriveInput):
        self.state_current_drive_input = msg

    def cb_tf_timer(self):
        """
        Called every timer_period. Publishes to self.publisher
        :return:
        """
        self.get_logger().debug("timer callback!")
        if self.state_current_drive_input is None:
            self.get_logger().debug("Exiting early since no drive received")
            self.tf_broadcaster.sendTransform(self.state_current_transform)
            self.pub_pivot_wheel.publish(self.state_current_pivot_wheel)
            return
        radius, direction = self.state_current_drive_input.radius, self.state_current_drive_input.direction
        speed = self.state_current_drive_input.speed

        # Linearly approximate our distance travelled based on the most recent control information
        signed_dist_m = MAX_VEL_M_S * speed * self.pub_rate_hz
        if self.state_current_drive_input.mode in [DriveInput.PIVOT, DriveInput.TANK]:
            dx, dy, dtheta = delta_pose_from_dist_radius(signed_dist_m, radius, direction)
        elif self.state_current_drive_input.mode == DriveInput.STRAFE:
            dx, dy, dtheta = 0.0, signed_dist_m, 0.0
        else:
            raise AttributeError(f"Invalid drive mode: {self.state_current_drive_input.mode}")
        self.apply_tf_offset(dx, dy, dtheta)

        # Immediately update wheel angles, assuming instant pivots for simplicity
        self.state_current_pivot_wheel.direction = direction
        self.state_current_pivot_wheel.radius = radius

        # Set timestamp on tf2 transform
        self.state_current_transform.header.stamp = self.get_clock().now().to_msg()
        self.get_logger().debug(f"Publishing transform: {self.state_current_transform}")

        self.tf_broadcaster.sendTransform(self.state_current_transform)
        self.pub_pivot_wheel.publish(self.state_current_pivot_wheel)

    def apply_tf_offset(self, dx, dy, dtheta):
        p, r, y = transform.quat_to_euler(self.state_current_transform.transform.rotation)
        self.state_current_transform.transform.translation.x += dx * np.cos(y) - dy * np.sin(y)
        self.state_current_transform.transform.translation.y += dx * np.sin(y) + dy * np.cos(y)
        y += dtheta
        self.state_current_transform.transform.rotation = transform.euler_to_quat((p, r, y))

    def initialise_transform(self):
        # initial map -> base_link
        initial_transform = TransformStamped()
        initial_transform.header.frame_id = 'map'
        initial_transform.header.stamp = self.get_clock().now().to_msg()
        initial_transform.child_frame_id = 'initial_base_link'
        initial_transform.transform.rotation.w = 1.0
        self.static_tf_broadcaster.sendTransform(initial_transform)

        tf = TransformStamped()
        tf.transform.rotation.z = 1.0
        tf.header.frame_id = 'initial_base_link'
        tf.header.stamp = self.get_clock().now().to_msg()
        tf.child_frame_id = 'base_link'
        self.state_current_transform = tf


def main():
    rclpy.init()
    node = DriveSimNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
