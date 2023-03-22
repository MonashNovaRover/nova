#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Listen to telemetry from the four wheels,
            and convert to wheel odometry, a set
            of two vectors for the two imaginary
            rover wheels on either side of the
            wheel base, which would give the
            same twist as the four real wheels
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: path_planner_node
TOPICS:
  - subscriber: /control/telemetry [Telemetry]
  - publisher: /localisation/wheel_odom [Vector]
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Max Tory, Aarushi Raheja
CREATION:	22/03/2023
EDITED:		22/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - AAAAAHHHHHH
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node

# example of how to import a custom message type
from core.msg import Telemetry, SingleTelemetry, WheelOdometry

import math
import numpy as np

CHASSIS_WIDTH = 0.7
CHASSIS_LENGTH = 0.84

ANGLE_OFFSET = math.atan2(CHASSIS_WIDTH, CHASSIS_LENGTH)

class TemplateNode(Node):

    def __init__(self):
        super().__init__("TemplateNode")
        # way-point publisher publishes a bunch of waypoints at once (hence using the 2D map datatype
        self.sub_telemetry = self.create_subscription(Telemetry, "/control/telemetry", self.telem_callback, 10)

        self.pub_odom = self.create_publisher(WheelOdometry, "/localisation/wheel_odom", 10)

        self.param_wheel_radius = self.declare_parameter("wheel_val_to_m_s", 1.496).value
        # resolver angles [-27306, 27306] as a fraction of max int16 [-32768, 32767]
        self.param_resolver_factor = self.declare_parameter("pivot_scale_factor", 0.8333).value
        self.param_wheel_offsets = self.declare_parameter("pivot_angle_offsets_rad", [-ANGLE_OFFSET, -ANGLE_OFFSET, -ANGLE_OFFSET, -ANGLE_OFFSET]).value
        self.param_wheels_reversed = self.declare_parameter("pivot_angle_reversed", [True, False, True, False]).value

        self.latest_telemetry = None

        self.pivot_angles_rad = np.zeros(4)
        self.wheel_vels_m_s = np.zeros(4)
        self.wheel_vectors = np.zeros((4, 2))

        timer_period = 0.05  # run the timer 10 times per second
        self.create_timer(timer_period, self.construct_vectors)

    def telem_callback(self, msg: Telemetry):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        self.latest_telemetry = msg

    def construct_vectors(self):
        """
        Constructs the two vectors for the two imaginary rover wheels on either side of the turning centre
        """
        if self.latest_telemetry is None:
            return
        # Get raw wheel velocities
        self.wheel_vels_m_s = [
            wheel.rotor_velocity * self.param_wheel_radius for wheel in self.latest_telemetry.wheels
        ]
        # get pivot angles
        self.pivot_angles_rad = [
            # reverse direction for front left and back right wheels
            (-1 if self.param_wheels_reversed[i] else 1) *\
            # rescale to radians
            (pivot.resolver_position / self.param_resolver_factor +\
            # Add offset
            self.param_wheel_offsets[i])
            for i, pivot in enumerate(self.latest_telemetry.pivots)
        ]

        print(self.pivot_angles_rad)
        # Construct vectors from velocities and angles
        self.wheel_vectors = np.array([[vel * math.cos(angle), vel * math.sin(angle)] for vel, angle in zip(self.wheel_vels_m_s, self.pivot_angles_rad)])
        print(self.wheel_vectors)

        # average left and right wheel vectors to get imagined wheel velocity beside the centre of the wheel base
        left_wheel_vector = np.mean(self.wheel_vectors[0:2], axis=0) 
        right_wheel_vector = np.mean(self.wheel_vectors[2:4], axis=0)

        print(left_wheel_vector, right_wheel_vector)

        wheel_odom = WheelOdometry()
        # Transforming to dumb frame
        wheel_odom.left_wheel_vel.z = -left_wheel_vector[0]
        wheel_odom.left_wheel_vel.x = -left_wheel_vector[1]
        wheel_odom.right_wheel_vel.z = -right_wheel_vector[0]
        wheel_odom.right_wheel_vel.x = -right_wheel_vector[1]

        self.pub_odom.publish(wheel_odom)


def main():
    rclpy.init()
    template_node = TemplateNode()
    rclpy.spin(template_node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
