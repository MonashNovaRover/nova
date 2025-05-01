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
from pynmeagps import NMEAReader, NMEAMessage
from pyrtcm import RTCMMessage

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
        self.fix_type = None

        ### Serial ###
        self.ser = Serial()
        self.config_port(self.port_name, self.baudrate)
        self.reader_nmea = NMEAReader(
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
        self.pub_pose = self.create_publisher(
            RoverPoseGPS, 
            '/gps_rover/fix', 
            10, 
        )
        self.pose = RoverPoseGPS()
        self.pose.header.frame_id = 'gps_rover'
        self.timer = self.create_timer(0, self.loop)

    def config_port(self, port_name : str, baudrate : int):
        self.ser.baudrate = baudrate
        if port_name == '':
           port_name = '/dev/ttyUSB0'
        self.ser.port = port_name
        self.ser.open()

    def sub_rtcm_callback(self, msg : UInt8MultiArray):
        msg_binary = bytes(msg.data)
        self.ser.write(msg_binary)
        
        ### LOG ###
        msg_log = f'''
            🛰️ RTCM3 Data:
            \traw: {msg_str}
        '''
        if self.fix_type == 3:
            self.get_logger().debug(msg_log, throttle_duration_sec=2)
        else:
            self.get_logger().warn(msg_log, throttle_duration_sec=2)


    def parse_nmea(self) -> None:
        self.pose.header.stamp = self.get_clock().now().to_msg()
        msg_raw : NMEAMessage
        try:
            msg_raw, msg_parsed = self.reader_nmea.read()
        except Exception as e:
            self.get_logger().warn(f'❌ Failed to read NMEA message: {e}')
            return

        self.get_logger().debug(f'✅ NMEA message received!', throttle_duration_sec=2)
        self.get_logger().debug(f'\t   raw NMEA message: {msg_raw}', throttle_duration_sec=2)
        self.get_logger().debug(f'\tparsed NMEA message: {msg_parsed}', throttle_duration_sec=2)

        if msg_parsed is None:
            self.get_logger().warn(f'❌ Failed to read NMEA message: \'msg_parsed\' cannot be None!', throttle_duration_sec=2)
            return

        if self.gps_module == 'skytraq':
            try:
                msg_str = str(msg_parsed)
                if msg_parsed.talker == 'P' and msg_parsed.msgID == 'STI' and msg_parsed.msgId == '036':
                    # We are dealing with a PSTI036 message, which contains orientation information
                    if msg_parsed.mode == 'R':
                        # RTK (Real-Time Kinematic) mode. We have valid heading
                        self.pose.heading_valid = True
                        self.pose.pitch, self.pose.roll, self.pose.yaw = msg_parsed.pitch, msg_parsed.roll, msg_parsed.heading
                    else:
                        # Not RTK mode. We don't have valid heading
                        self.pose.heading_valid = False
                elif msg_parsed.talker == 'GN' and msg_parsed.msgID == 'RMC':
                    if msg_parsed.status == 'A':
                        # Valid
                        self.pose.valid = True
                        self.pose.latitude, self.pose.longitude = msg_parsed.lat, msg_parsed.lon
                    else:
                        self.pose.valid = False
                elif msg_parsed.talker == 'GP' and msg_parsed.msgID == 'GGA':
                    self.fix_type = msg_parsed.quality   # 1 = No fix, 2 = 2D fix, 3 = 3D fix

                ### ROS2 ###
                self.pub_pose.publish(self.pose)

                ### LOG ###
                msg_log = f'''
                    🛰️ NMEA Data:
                    \traw: {msg_str}
                    \tvalid: {self.pose.valid}
                    \tfix type: {'None' if self.fix_type == 1 else '2D' if self.fix_type == 2 else '3D' if self.fix_type == 3 else self.fix_type}
                    \tlat: {self.pose.latitude:8.3f}
                    \tlon: {self.pose.longitude:8.3f}
                    \tpitch: {self.pose.pitch:8.2f}
                    \troll: {self.pose.roll:8.2f}
                    \tyaw: {self.pose.yaw:8.2f}
                '''
                if self.pose.valid:
                    self.get_logger().debug(msg_log, throttle_duration_sec=2)
                else:
                    self.get_logger().warn(msg_log, throttle_duration_sec=2)

            except Exception as e:
                self.get_logger().warn(f'❌ Error: {e}, Bad message: {msg_parsed}')

    def loop(self) -> None:
        self.parse_nmea()

def main (args = None):
    rclpy.init(args = args)
    node = GPSRover()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()