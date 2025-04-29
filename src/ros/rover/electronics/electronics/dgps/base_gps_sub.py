#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Takes RTCM error correction data from 
base (ublox) GPS and writes data to the rover 
(skytraq) GPS over USB.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: SubToBaseNode
TOPICS:
  - subscriber: /gps_base/rtcm  [UInt8MultiArray]
  - publisher: /gps_rover/fix   [RoverPoseGPS]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Shelby N, Victor Bartlinski
CREATION:	25/02/2023
EDITED:		25/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - check if buffer clearing is necessary
 - convert log to debug
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
import serial

import rclpy
from rclpy.node import Node

from std_msgs.msg import UInt8MultiArray
from rclpy.logging import LoggingSeverity

from rclpy.qos import qos_profile_sensor_data as qos

from nova_interfaces.msg import RoverPoseGPS
import logging

class SubToBaseNode(Node):
    def __init__(self, com_no, baud):
        super().__init__('getBaseCorrection_pub')

        self.ser = serial.Serial()

        self.declare_parameter('dev', '/dev/ttyUSB0')
        self.declare_parameter('baud_rate', '115200')

        self.config_port()

        self.count = 0

        self.subscription = self.create_subscription(
            UInt8MultiArray,
            'gps_base/rtcm3', 
            self.callback_func,
            qos)

        self.get_logger().info('base_gps_sub started.')


    def callback_func(self, msg):
        
        # if self.count > 50:                 # need to test w/out, error may have been fixed by different section
        #     self.count = 0
        #     self.ser.reset_output_buffer()
        # self.count += 1
        
        raw_rtcm_msg = bytes(msg.data)
        self.ser.write(raw_rtcm_msg)
        self.get_logger().debug(f'raw bytes: {raw_rtcm_msg}',throttle_duration_sec=2)

    def config_port(self):
        port_name = self.get_parameter('dev').value
        baud_rate = self.get_parameter('baud_rate').value

        self.ser.baudrate = baud_rate
        #if port_name == '':
        #    port_name = '/dev/ttyUSB1'
        self.ser.port = port_name
        self.ser.open()

def main (args = None):
    rclpy.init(args = args)
    subscriber = SubToBaseNode('', 115200)
    rclpy.spin(subscriber)

    subscriber.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()

