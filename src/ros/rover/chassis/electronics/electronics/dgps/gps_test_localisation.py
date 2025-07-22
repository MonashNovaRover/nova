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
  - publisher: /fix             [NavSatFix]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Shelby N, Victor Bartlinski
CREATED:	25/02/2023
EDITED:		30/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Check if buffer clearing is necessary
 - Abstract serial protocol parsing to a function 
   for each protocol; only STI messages under the 
   'skytraq' module condition. 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from serial import Serial
from pynmeagps import NMEAReader, NMEAMessage
from pyrtcm import RTCMMessage
import re
import pandas as pd

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data as qos
from rclpy.logging import LoggingSeverity

from std_msgs.msg import UInt8MultiArray
from nova_interfaces.msg import RoverPoseGPS
from sensor_msgs.msg import NavSatFix, NavSatStatus
import logging

class GPSRover(Node):
    def __init__(self):
        super().__init__('gps_rover')

        self.get_logger().debug(f'Configuring node...')

        self.baudrate = self.declare_parameter(
            name='baudrate', 
            value=115200, 
        ).value
        self.port_name = self.declare_parameter(
            name='port_name', 
            value='/dev/ttyUSB0', 
        ).value
        self.gps_module = self.declare_parameter(
            name='gps_module', 
            value='skytraq', 
        ).value
        self.publisher_rate = self.declare_parameter(
            name='publisher_rate', 
            value=1, 
        ).value
        self.fix_type = None

        ### Data ###
        data = pd.read_csv('data-1.csv',header=None)
        data = data.iloc[:, -2:]
        data.columns = ['lat', 'lon']
        # Add all points
        self.lat = []
        self.lon = []
        self.i = 0
        for _, row in data.iterrows():
            self.lat.append(row['lat'])
            self.lon.append(row['lon'])

        ### ROS2 ###
        self.pub_navsatfix = self.create_publisher(
            NavSatFix, 
            '/fix', 
            10, 
        )
        self.timer = self.create_timer(1/self.publisher_rate, self.loop)

        self.get_logger().debug(f'Node configured!')

    def pub_navsatfix_callback(self):
        '''
        https://docs.ros.org/en/api/sensor_msgs/html/msg/NavSatFix.html
        https://docs.ros.org/en/api/sensor_msgs/html/msg/NavSatStatus.html
        '''
        msg = NavSatFix()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = 'gps'
        msg.status = NavSatStatus()
        msg.status.status = NavSatStatus.STATUS_FIX # valid fix
        msg.status.service = NavSatStatus.SERVICE_GPS # using GPS
        msg.latitude = self.lat[self.i]
        msg.longitude = self.lon[self.i]
        msg.altitude = 0.
        # msg.position_covariance = [
        #     0.0, 0.0, 0.0,
        #     0.0, 0.0, 0.0,
        #     0.0, 0.0, 0.0,
        # ]
        msg.position_covariance_type = NavSatFix.COVARIANCE_TYPE_UNKNOWN
        self.i += 1
        self.pub_navsatfix.publish(msg)

    def loop(self) -> None:
        self.pub_navsatfix_callback()

def main (args = None):
    rclpy.init(args = args)
    node = GPSRover()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()