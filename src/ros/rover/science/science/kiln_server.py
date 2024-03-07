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

    KILN_CARD_SEND_IDS = [0x0A0, 0x0B0]
    KILN_POWER_COMMAND = 0x07
    KILN_OFF = 0x00 
    KILN_ON = 0xFF
    KILN_TEMP_FEEDBACK_ID = 0x4B3

    def convert_from_thermistor_(reading):
        return 1.0*reading

    def convert_from_IR_(reading):
        return reading*0.02 - 273.15

    def __init__(self):
        super().__init__('kiln_server')
        
        self.get_logger().set_level(logging.DEBUG)
        self.get_logger().info("Kiln server starting")

        #subscriber to polling status
        self.service = self.create_service(KilnCommand, '/science/kiln_command', self.command_callback)
        self.get_logger().info("Kiln service created")
        #publisher to publish the data from the kilns.
        self.publisher = self.create_publisher(KilnData, "/science/kiln_data", 10)

        #initialise the can bus
        self.bus = jcan.Bus()

        # Set filter IDs and callbacks.
        self.bus.set_id_filter(KilnServer.KILN_CARD_SEND_IDS + [KilnServer.KILN_TEMP_FEEDBACK_ID])
        self.bus.add_callback(KilnServer.KILN_TEMP_FEEDBACK_ID, self.update_temp)

        #create timers
        self.can_spin_timer = self.create_timer(0.05, self.bus.spin)
        self.send_can_timer = self.create_timer(0.2, self.send_can_command)
        self.publish_data_timer = self.create_timer(1, self.publish_data)

        self.temp = [0.0, 0.0, 0.0]
        self.is_on = False

        self.bus.open("can1")

    def command_callback(self, request, response):
        try:
            if request.state:   # turn on kiln
                self.get_logger().info("Kiln try On")
                for id in KilnServer.KILN_CARD_SEND_IDS:
                    kiln_frame = jcan.Frame(id , [KilnServer.KILN_POWER_COMMAND, KilnServer.KILN_ON])
                    self.bus.send(kiln_frame)
                self.is_on = True
            else:               # turn off kiln
                self.get_logger().info("Kiln try Off")
                for id in KilnServer.KILN_CARD_SEND_IDS:
                    kiln_frame = jcan.Frame(id , [KilnServer.KILN_POWER_COMMAND, KilnServer.KILN_OFF])
                    self.bus.send(kiln_frame)
                self.is_on = False
            self.get_logger().info(f"Kiln Status = {self.is_on}")
            response.success = True
        except Exception as e:
            self.get_logger().info(str(e))
            response.success = False

        return response

    def send_can_command(self):
        try:
            if self.is_on:
                for id in KilnServer.KILN_CARD_SEND_IDS:
                    kiln_frame = jcan.Frame(id , [KilnServer.KILN_POWER_COMMAND, KilnServer.KILN_ON])
                    self.bus.send(kiln_frame)
            else:
                for id in KilnServer.KILN_CARD_SEND_IDS:
                    kiln_frame = jcan.Frame(id , [KilnServer.KILN_POWER_COMMAND, KilnServer.KILN_OFF])
                    self.bus.send(kiln_frame)
        except Exception as e:
            self.get_logger().info(str(e))

    def update_temp(self, frame):
        self.get_logger().info("Kiln try update temp")
        sensor_id = frame.data[0] - 1 
        if 0 <= sensor_id <= 2:
            reading = frame.data[1] * 2**8 + frame.data[2]  # as reading is return as two bytes (8 bit integer)
            if sensor_id == 2:
                self.temp[sensor_id] = KilnServer.convert_from_IR_(reading)
                self.get_logger().info(f"IR reading updated to {self.temp[sensor_id]} using {reading}")
            else:
                self.temp[sensor_id] = KilnServer.convert_from_thermistor_(reading)
                self.get_logger().info(f"Thermistor {sensor_id} reading updated to {self.temp[sensor_id]} using {reading}")

    def publish_data(self):
        msg = KilnData()
        msg.temp = self.temp
        msg.state = self.is_on
        self.publisher.publish(msg)
        self.get_logger().info(f"Temps [{self.temp[0]}, {self.temp[1]} , {self.temp[2]}] and state {self.is_on} published")


def main():
    rclpy.init()
    server_node = KilnServer()
    rclpy.spin(server_node)
    server_node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()