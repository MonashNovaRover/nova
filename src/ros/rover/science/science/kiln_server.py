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
    # Jono Card IDs
    KILN_CARD_SEND_IDS = [0x0A0, 0x0B0]
    KILN_TEMP_FEEDBACK_ID = 0x4B3
    # Kiln Command
    KILN_POWER_COMMAND = 0x07
    # Kiln Power States
    KILN_OFF = 0x00 
    KILN_ON = 0xFF
    # Kiln Sensor IDs
    KILN_SENSOR_IDS = [0x01, 0x02, 0x03]
    # ROS Params
    CAN_BUS_PARAM = "can_bus"
    KILN_TEMP_CONVERSION_PARAM = "science_temp_conversion"
    # ROS Topics
    KILN_DATA_TOPIC = "/science/kiln_data"
    # ROS Services
    KILN_COMMAND_SERVICE = "/science/kiln_command"

    def __init__(self):
        super().__init__('kiln_server')
        
        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Kiln server starting")
        self.declare_parameter(KilnServer.KILN_TEMP_CONVERSION_PARAM, False)

        #subscriber to polling status
        self.service = self.create_service(KilnCommand, KilnServer.KILN_DATA_TOPIC, KilnServer.command_callback)
        self.get_logger().info("Kiln service created")
        #publisher to publish the data from the kilns.
        self.publisher = self.create_publisher(KilnData, KilnServer.KILN_COMMAND_SERVICE, 10)

        #initialise the can bus
        self.bus = jcan.Bus()

        # Set filter IDs and callbacks.
        self.bus.set_id_filter(KilnServer.KILN_CARD_SEND_IDS + [KilnServer.KILN_TEMP_FEEDBACK_ID])
        self.bus.add_callback(KilnServer.KILN_TEMP_FEEDBACK_ID, self.update_temp)

        #create timers
        self.can_spin_timer = self.create_timer(0.05, self.bus.spin)
        self.send_can_timer = self.create_timer(0.2, self.send_can_command)
        self.publish_data_timer = self.create_timer(1, self.publish_data)

        self.temp = [0, 0, 0]
        self.is_on = False

        self.bus.open(self.get_parameter(KilnServer.CAN_BUS_PARAM).value)
    
    def convert(self, reading):
        if self.get_parameter(KilnServer.KILN_TEMP_CONVERSION_PARAM).value:
            return int(reading*0.02 - 273.15)
        return reading


    def send_kiln_on(self):
        self.get_logger().info("Send Kiln ON")
        for id in KilnServer.KILN_CARD_SEND_IDS:
            kiln_frame = jcan.Frame(id , [KilnServer.KILN_POWER_COMMAND, KilnServer.KILN_ON])
            self.bus.send(kiln_frame)
        

    def send_kiln_off(self):
        self.get_logger().info("Send Kiln OFF")
        for id in KilnServer.KILN_CARD_SEND_IDS:
            kiln_frame = jcan.Frame(id , [KilnServer.KILN_POWER_COMMAND, KilnServer.KILN_OFF])
            self.bus.send(kiln_frame)


    def command_callback(self, request, response):
        try:
            if request.state:   # turn on kiln
                self.send_kiln_on()
                self.is_on = True
            else:               # turn off kiln
                self.send_kiln_off()
                self.is_on = False
            self.get_logger().info(f"Kiln Status = {self.is_on}")
            response.success = True
        except Exception as e:
            self.get_logger().error(f"Failed to process Kiln service request: {str(e)}")
            response.success = False

        return response

    def send_can_command(self):
        try:
            if self.is_on:
                self.send_kiln_on()
            else:
                self.send_kiln_off()
        except Exception as e:
            self.get_logger().error(f"Failed to send kiln CAN command: {str(e)}")

    def update_temp(self, frame):
        try: 
            self.get_logger().info("Kiln try update temp")
            sensor_id = frame.data[0]
            sensor_index = sensor_id - 1
            if sensor_id in KilnServer.KILN_SENSOR_IDS:
                reading = frame.data[1] * 2**8 + frame.data[2]  # as reading is returned as two bytes (16 bit integer)
                self.temp[sensor_index] = self.convert(reading)
                self.get_logger().info(f"Sensor {sensor_id} reading updated to {self.temp[sensor_index]} using {reading}")
            else:
                self.get_logger().info(f"Sensor {sensor_id} not in list of sensors")
        except Exception as e:
            self.get_logger().error(f"Failed to update temp: {str(e)}")

    
    def publish_data(self):
        try: 
            msg = KilnData()
            msg.temp = self.temp
            msg.state = self.is_on
            self.publisher.publish(msg)
            self.get_logger().info(f"Temps [{str(self.temp)}] and state {self.is_on} published")
        except Exception as e:
            self.get_logger().error(f"Failed to publish data: {str(e)}")

def main():
    rclpy.init()
    server_node = KilnServer()
    rclpy.spin(server_node)
    server_node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()