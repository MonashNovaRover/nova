#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
<insert purpose here>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: Auto
TOPICS:
  - publisher: <topic> [<msg type>]
SERVICES:
	- service: <service> [<srv type>]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        <package>
AUTHOR(S):      <insert your name>
CREATION:       <current date>
EDITED:         <current date>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
from nova_interfaces.msg import GPSData

class Auto(Node):

    def __init__(self):
        super().__init__("Auto")

        # define variables
        self.PERIOD = 0.5
        self.speed = 0.0000002 #lat-long/s
        self.current_pos = (38.406787449642906, -110.79183322459582)
        self.target_pos = (38.408091100083226, -110.79039033258813)

        # Creates a ROS2 timer that calls the update function every interval
        self.update_timer = self.create_timer(self.PERIOD, lambda: self.update(self.PERIOD))

        # add publishers
        self.gps_publisher = self.create_publisher(GPSData, '/gps_rover/fix_custom', 10)

        # add services


        self.get_logger().info(f"{self.get_name()} started.")

    def update(self, delta_time):
        delta_position = (
            self.target_pos[0] - self.current_pos[0],
            self.target_pos[1] - self.current_pos[1]
        )

        distance = (delta_position[0] ** 2 + delta_position[1] ** 2) ** 0.5

        if distance > 0:
            # figure out the direction to move

            # figure out how far in that direction to move

            #
            pass




        self.current_pos += displacement

        # Construct the message to send
        msg = GPSData()
        msg.latitude, msg.longitude = 38.406787449642906, -110.79183322459582

        # Publish message
        self.gps_publisher.publish(msg)


def main():
    rclpy.init()
    node = Auto()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
