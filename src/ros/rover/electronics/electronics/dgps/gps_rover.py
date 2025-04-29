#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Takes RTCM error correction data from 
base (ublox) GPS and writes data to the rover 
(skytraq) GPS over USB.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gps_rover
TOPICS:
  - subscriber: /gps_base/rtcm  [UInt8MultiArray]
  - publisher: /gps_rover/fix   [RoverPoseGPS]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Shelby N, Victor Bartlinski
CREATED:	25/02/2023
EDITED:		30/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - check if buffer clearing is necessary
 - convert log to debug
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from serial import Serial

import rclpy
from rclpy.node import Node

from std_msgs.msg import UInt8MultiArray
from rclpy.logging import LoggingSeverity

from rclpy.qos import qos_profile_sensor_data as qos

from nova_interfaces.msg import RoverPoseGPS
import logging

class GPSRover(Node):
    def __init__(self):
        super().__init__('gps_rover')
        self.baudrate = self.declare_parameter('baudrate', '115200').value
        self.port_name = self.declare_parameter('port_name', '/dev/ttyUSB0').value

        ### Serial ###
        self.ser = Serial()
        self.config_port(self.port_name, self.baudrate)
        self.nmea_reader = NMEAReader(
            self.ser, 
            validate=0x03,  # validate both checksum and message id
            nmeaonly=True,  # Raise an error on receiving a badly formatted message
        )

        ### ROS2 ###
        self.sub_rtcm = self.create_subscription(
            UInt8MultiArray, 
            'gps_base/rtcm', 
            self.sub_rtcm_callback, 
            qos, 
        )

        self.get_logger().info('gps_rover started.')

    def sub_rtcm_callback(self, msg : UInt8MultiArray):
        msg_rtcm = bytes(msg.data)
        self.ser.write(msg_rtcm)
        self.get_logger().debug(f'RTCM3: {msg_rtcm}', throttle_duration_sec=2)

    def config_port(self, port_name : str, baudrate : int):
        self.ser.baudrate = baudrate
        if port_name == '':
           port_name = '/dev/ttyUSB0'
        self.ser.port = port_name
        self.ser.open()

def main (args = None):
    rclpy.init(args = args)
    node = GPSRover()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()