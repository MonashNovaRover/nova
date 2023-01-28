#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: To simulate wheel velocities being sent
to simulate T265 movement without being on Rover
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: wheel_sim
TOPICS:
  - publisher: /electronics/wheel_data [core.msg.WheelData]
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Liam Whittle
CREATION:	13/03/2022
EDITED:		13/03/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
from core.msg import WheelData


class WheelSim(Node):
    def __init__(self):
        super().__init__("wheel_sim")
        self.publisher = self.create_publisher(WheelData, "/electronics/wheel_data", 10)
        timer_period = 0.1  # run the timer 10 times per second
        self.create_timer(timer_period, self.timer_callback)

    def timer_callback(self):
        """
        Publishing constant positive velocities of 10 cm per second
        :return:
        """
        wheel_data = WheelData()
        wheel_data.velocities[0] = .1
        wheel_data.velocities[1] = .1
        wheel_data.velocities[2] = .1
        wheel_data.velocities[3] = .1
        wheel_data.velocities[4] = .1
        wheel_data.velocities[5] = .1
        self.publisher.publish(wheel_data)

def main():
    rclpy.init()
    wheel_sim = WheelSim()
    rclpy.spin(template_node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
