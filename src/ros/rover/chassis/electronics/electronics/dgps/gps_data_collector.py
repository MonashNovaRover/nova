#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Takes RTC corrected GPS locations of both
rover and base station and saves the data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gps_rover
TOPICS:
  - subscriber: /gps_base/fix   [NavSatFix]
  - subscriber: /gps_rover/fix  [NavSatFix]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Will Middlewick
CREATED:	06/05/2025
EDITED:		06/05/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from serial import Serial
from pynmeagps import NMEAReader, NMEAMessage
from pyrtcm import RTCMMessage

import rclpy
from rclpy.node import Node

from std_msgs.msg import UInt8MultiArray
from rclpy.logging import LoggingSeverity

from rclpy.qos import qos_profile_sensor_data as qos

from sensor_msgs.msg import NavSatFix
import logging

import csv
from datetime import datetime

class GPSDataCollector(Node):
    def __init__(self):
        super().__init__('gps_data_collector')

        ### ROS2 ###
        self.sub_rtcm = self.create_subscription(
            NavSatFix, 
            'gps_base/fix', 
            self.sub_base_callback, 
            qos, 
        )
        self.sub_rtcm = self.create_subscription(
            NavSatFix, 
            'gps_rover/fix', 
            self.sub_rover_callback, 
            qos, 
        )

        self.base_lat = 0
        self.base_lon = 0
        self.rover_lat = 0
        self.rover_lon = 0

        self.timer = self.create_timer(0.5, self.loop) # start mainloop for the node
        self.get_logger().set_level(rclpy.logging.LoggingSeverity.INFO)

    def sub_base_callback(self, msg : NavSatFix):
        self.base_lat = msg.latitude
        self.base_lon = msg.longitude

    def sub_rover_callback(self, msg : NavSatFix):
        self.rover_lat = msg.latitude
        self.rover_lon = msg.longitude

    def save_data(self) -> None:
        timestamp = datetime.now().isoformat()

        # write the data to a csv
        with open('data.csv', mode='a', newline='') as file:
            writer = csv.writer(file)
            writer.writerow([timestamp, self.base_lat, self.base_lon, self.rover_lat, self.rover_lon])

        self.get_logger().info(f'Written to data.csv')

    def loop(self) -> None:
        self.save_data()

def main (args = None):
    rclpy.init(args = args)
    node = GPSDataCollector()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()