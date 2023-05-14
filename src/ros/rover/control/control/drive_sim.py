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
 - All
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
# ros imports
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from tf2_ros import TransformBroadcaster

# msg imports
from core.msg import DriveInput, PivotWheelData
from geometry_msgs.msg import Transform, TransformStamped

# python imports 
from typing import Tuple
import numpy as np

MAX_VEL_M_S = 12


def steer_to_radius_direction(steer: float) -> Tuple(float, int):
    """
    Take a steer value like what is given by the controller in the range [-1, 1]
    and map it to a radius in the range [0, inf] and a direction {-1, 1}
    """
    assert steer <= 1 and steer >= -1, f"Steer {steer} not in required range"
    direction = np.sign(steer)
    if steer = 0:
        radius = float('inf')
    else:
        abs_steer = np.abs(steer)
        radius = (1 / abs_steer) - 1

    return radius, direction


def delta_pose_from_dist_radius(signed_dist: float, radius: float, turn_dir: int) -> Tuple(float, float, float):
    """
    Take a signed distance and radius, and assuming we have moved that distance around
    the circumference of a circle, return the dx, dy, and dtheta associated
    with this transformation. Works for any units so long as the dist and radius are given
    in the same units
    :returns dx - distance in forward direction
    :returns dy - distance in left direction
    :returns dtheta - radians
    """
    if signed_dist > np.pi * signed_radius or signed_radius == 0:
        # Very tight turn
        return 0.0, 0.0, 2.0 * turn_dir
    dist_sign = np.sign(dist)
    dist_abs = np.abs(dist)

    rad_sign = turn_dir
    rad_abs = radius

    # radians are cool
    dtheta_abs = dist_abs / rad_abs
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
        self.subscriber = self.create_subscription(DriveInput, "/control/autonomous_commands", self.cb_drive_sub, self.qos)

        self.tf_broadcaster = TransformBroadcaster(self)
        self.pub_pivot_wheel = self.create_publisher(PivotWheelData, "/control/pivot_wheel", 10)

        self.state_current_transform : TransformStamped = None
        self.state_current_drive_input : DriveInput = None

        pub_rate_hz = 0.1  # run the timer 10 times per second
        self.timer_pub_tf = self.create_timer(timer_pub_tf, self.cb_tf_timer)

    def cb_drive_sub(self, msg: DriveInput):
        self.state_current_drive_input = msg

    def cb_tf_timer(self):
        """
        Called every timer_period. Publishes to self.publisher
        :return:
        """
        string_msg = String()
        string_msg.data = "Rover's x coordinate: " + str(self.msg.x)
        self.publisher.publish(string_msg)


def main():
    rclpy.init()
    node = DriveSimNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
