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
AUTHOR(S):      Connor Macdougall, Tash Lee
CREATION:       29/02/2024
EDITED:         03/02/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import logging
import rclpy
from rclpy.node import Node
import jcan

from nova_interfaces.msg import KilnData
from nova_interfaces.srv import KilnCommand


class KilnServer(Node):
    # Jono Card IDs
    KILN_CARD_SEND_IDS = [0x0A0]
    KILN_TEMP_FEEDBACK_ID = 0x4E0
    # Kiln Command
    KILN_POWER_COMMAND = 0x07
    # Kiln Power States
    KILN_OFF = 0x00 
    KILN_ON = 0xFF
    # Kiln Sensor IDs
    KILN_SENSOR_IDS = [0x03]
    # ROS Params
    CAN_BUS_PARAM = "can_bus"
    KILN_TEMP_CONVERSION_PARAM = "science_temp_conversion"
    # ROS Topics
    KILN_DATA_TOPIC = "/science/kiln_data"
    # ROS Services
    KILN_COMMAND_SERVICE = "/science/kiln_command"
    # Default target
    DEFAULT_TARGET_TEMP = 25

    def __init__(self):
        super().__init__('kiln_server')
        
        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Kiln Server starting")

        # The calculations are currently performed on the arduino side to this is set to false
        # If the calculations are to be performed on the ROS side, this should be set to true
        self.declare_parameter(KilnServer.CAN_BUS_PARAM, "can1")
        self.declare_parameter(KilnServer.KILN_TEMP_CONVERSION_PARAM, False)

        #subscriber to polling status
        self.service = self.create_service(KilnCommand, KilnServer.KILN_COMMAND_SERVICE, self.command_callback)
        
        #publisher to publish the data from the kilns.
        self.publisher = self.create_publisher(KilnData, KilnServer.KILN_DATA_TOPIC, 10)

        #initialise the can bus
        self.bus = jcan.Bus()

        # Set filter IDs and callbacks.
        self.bus.set_id_filter(KilnServer.KILN_CARD_SEND_IDS + [KilnServer.KILN_TEMP_FEEDBACK_ID])
        self.bus.add_callback(KilnServer.KILN_TEMP_FEEDBACK_ID, self.update_temp)

        #create timers
        self.can_spin_timer = self.create_timer(0.05, self.bus.spin)
        self.send_can_timer = self.create_timer(0.2, self.send_can_command)
        self.publish_data_timer = self.create_timer(1, self.publish_data)

        self.temp = [0]
        self.is_on = False
        self.target = self.DEFAULT_TARGET_TEMP

        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)

        self.get_logger().info(f"Kiln Server started on {self.get_parameter(self.CAN_BUS_PARAM).value}")

    def convert(self, reading: int):
        """
        Converts the reading from the kiln to the correct temperature
        IS PERFORMED ONLY IF THE PARAMETER IS SET TO TRUE
        """
        if self.get_parameter(KilnServer.KILN_TEMP_CONVERSION_PARAM).value:
            return int(reading*0.02 - 273.15)
        return reading


    def send_kiln_on(self):
        """
        Sends the kiln ON CAN commands to the kiln
        """
        self.get_logger().debug("Send Kiln ON")
        for id in KilnServer.KILN_CARD_SEND_IDS:
            kiln_frame = jcan.Frame(id , [KilnServer.KILN_POWER_COMMAND, KilnServer.KILN_ON])
            self.bus.send(kiln_frame)
        

    def send_kiln_off(self):
        """
        Sends the kiln OFF CAN commands to the kiln
        """
        self.get_logger().debug("Send Kiln OFF")
        for id in KilnServer.KILN_CARD_SEND_IDS:
            kiln_frame = jcan.Frame(id , [KilnServer.KILN_POWER_COMMAND, KilnServer.KILN_OFF])
            self.bus.send(kiln_frame)


    def command_callback(self, request, response):
        """
        Callback for the kiln command service
        """
        try:
            self.get_logger().info(f"Kiln service request received: {request}")
            if request.state:
                # Turn on the kiln
                self.send_kiln_on()
                self.is_on = True
                self.target = request.target
                self.get_logger().info(f"Kiln target temp updated to: {self.target}")
            else:
                # Turn off the kiln
                self.send_kiln_off()
                self.is_on = False
            self.get_logger().info(f"Kiln Status = {self.is_on}")
            response.success = True
        except Exception as e:
            self.get_logger().error(f"Failed to process Kiln service request: {str(e)}")
            response.success = False

        return response

    def send_can_command(self):
        """
        Sends the commanded state of the kiln to the kiln via CAN
        Performs this continuously otherwise the kiln will turn off
        """
        try:
            if self.is_on and self.target>self.temp[0]:
                self.send_kiln_on()
            else:
                self.send_kiln_off()
        except Exception as e:
            self.get_logger().error(f"Failed to send kiln CAN command: {str(e)}")

    def update_temp(self, frame: jcan.Frame):
        """
        Updates the temperature of the kiln
        """
        try: 
            self.get_logger().debug("Kiln temp feedback received")
            self.get_logger().debug(f"Frame: {frame}")
            sensor_id = frame.data[0]
            for i in range(len(KilnServer.KILN_SENSOR_IDS)):
                if KilnServer.KILN_SENSOR_IDS[i] != sensor_id:
                    continue
                reading = frame.data[1] # * 2**8 + frame.data[2]  # as reading is returned as two bytes (16 bit integer)
                self.temp[i] = self.convert(reading)
                self.get_logger().debug(f"Sensor {sensor_id} reading updated to {self.temp[i]} using {reading}")
            else:
                self.get_logger().debug(f"Sensor {sensor_id} not in list of sensors")
        except Exception as e:
            self.get_logger().error(f"Failed to update temp: {str(e)}")
    
    def publish_data(self):
        """
        Publishes the current temperature and state of the kiln
        """
        try: 
            msg = KilnData()
            msg.temp = self.temp
            msg.state = self.is_on
            self.publisher.publish(msg)
            self.get_logger().debug(f"Temps [{str(self.temp)}] and state {self.is_on} published")
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
