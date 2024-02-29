#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
Handles enacting commands received over ROS and feedback received through CAN
for the kiln.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: kiln_server
TOPICS:
    - /science/kiln_data                    [pub]
SERVICES:
    - /science/kiln_command              [server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Connor Macdougall
CREATION:       29/02/2024
EDITED:         30/02/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
import jcan

from core.msg import KilnData
from core.srv import KilnCommand


class KilnServer(Node):

    def __init__(self):
        super().__init__('kiln_server')

        #subscriber to polling status
        self.service = self.create_service(KilnCommand, '/science/kiln_command', self.command_callback)
        #publisher to publish the data from the kilns.
        self.publisher = self.create_publisher(KilnData, "/science/kiln_data", 1)

        #initialise the can bus
        self.bus = jcan.Bus()

        # Set filter IDs and callbacks.
        self.bus.set_id_filter([0x4B3])
        self.bus.add_callback(0x4B3, self.update_temp)

        #create timers
        self.can_spin_timer = self.create_timer(0.05, self.bus.spin)
        self.publish_data_timer = self.create_timer(1, self.publish_data)

        self.temp = [0, 0, 0]
        self.is_on = False

        self.bus.open("can1")

    def command_callback(self):
        pass

    def update_temp(self):
        pass

    def publish_data(self):
        pass


def main():
    rclpy.init()
    server_node = KilnServer()
    rclpy.spin(server_node)
    server_node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()