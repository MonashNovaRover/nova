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

import logging
import rclpy
from rclpy.node import Node
import jcan

from core.msg import KilnData
from core.srv import KilnCommand


class KilnServer(Node):

    def __init__(self):
        super().__init__('kiln_server')
        self.get_logger().set_level(logging.DEBUG)
        self.get_logger().info("Kiln server starting")

        #subscriber to polling status
        self.service = self.create_service(KilnCommand, '/science/kiln_command', self.command_callback)
        #publisher to publish the data from the kilns.
        self.publisher = self.create_publisher(KilnData, "/science/kiln_data", 1)

        #initialise the can bus
        self.bus = jcan.Bus()

        # Set filter IDs and callbacks.
        self.bus.set_id_filter([0x4B3, 0x0A0, 0x0B0])
        self.bus.add_callback(0x4B3, self.update_temp)

        #create timers
        self.can_spin_timer = self.create_timer(0.05, self.bus.spin)
        self.publish_data_timer = self.create_timer(1, self.publish_data)

        self.temp = [0, 0, 0]
        self.is_on = False

        self.bus.open("can1")

    def command_callback(self, request, response):
        try:
            if request.state:   # turn on kiln
                self.get_logger().info("Kiln try On")
                for i in range(10,12):
                    kiln_frame = jcan.Frame( i << 4 , [7, 255])
                    self.bus.send(kiln_frame)
                self.is_on = True
                self.get_logger().info("Kiln On")
            else:               # turn off kiln
                self.get_logger().info("Kiln try Off")
                for i in range(10,12):
                    kiln_frame = jcan.Frame( i << 4 , [7, 0])
                    self.bus.send(kiln_frame)
                self.is_on = False
                self.get_logger().info("Kiln Off")
            response.success = True
        except:
            response.success = False

        return response

    def update_temp(self, frame):
        self.get_logger().info("Kiln try update temp")
        sensor_id = frame.data[0] - 1
        if 0 <= sensor_id <= 2:
            reading = frame.data[1]
            if sensor_id == 2:
                self.temp[sensor_id] = reading*0.02 - 273.15
                self.get_logger().info("IR reading updated")
            else:
                self.temp[sensor_id] = reading
                self.get_logger().info("Thermistor reading updated")

    def publish_data(self):
        msg = KilnData()
        msg.temp = self.temp
        msg.state = self.is_on
        self.publisher.publish(msg)
        self.get_logger().info("Temps and state published")


def main():
    rclpy.init()
    server_node = KilnServer()
    rclpy.spin(server_node)
    server_node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()