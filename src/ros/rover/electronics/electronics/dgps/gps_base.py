#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Reads RTCM3 error correction data from 
base (ublox) GPS and publishes to rover (skytraq) 
GPS.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: GPSBase
TOPICS:
  - publisher: /gps_base/fix    [RoverPoseGPS]
  - publisher: /gps_base/rtcm   [UInt8MultiArray]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Shelby N, Will Middlewick, Victor 
            Bartlinski
CREATION:	25/02/2023
EDITED:		16/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - convert log to debug
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from serial import Serial
from pynmeagps import NMEAReader, NMEAMessage
from pyrtcm import RTCMReader, RTCMMessage
import re
import rclpy
from rclpy.node import Node
from std_msgs.msg import UInt8MultiArray

from nova_interfaces.msg import RoverPoseGPS
import logging

GPS_MODULE = 'ublox'

class GPSBase(Node):
    def __init__ (self, com_no, baud):
        super().__init__('gps_base')
        self.param_max_calibration_error = self.declare_parameter('max_calibration_error_degrees', 5e-5).value

        self.pose : RoverPoseGPS = RoverPoseGPS()
        self.pose.header.frame_id = 'gps_base'

        self.fix_type : str = None

        self.ser : Serial = Serial()
        self.config_port(com_no, baud)
        self.nmea_reader = NMEAReader(
            self.ser,
            validate=0x03,  # validate both checksum and message id
            nmeaonly=True   # Raise an error on receiving a badly formatted message
        )
        self.rtcm_reader = RTCMReader(
            self.ser,
            validate=0x01,  # validate checksum
        )

        self.nmea_publisher = self.create_publisher(RoverPoseGPS, '/gps_base/fix', 10)
        self.rtcm_publisher = self.create_publisher(UInt8MultiArray, '/gps_base/rtcm', 10)
        self.timer = self.create_timer(0, self.publisher_callback)

    def parse_msg(self):
        
        self.pose.header.stamp = self.get_clock().now().to_msg()
        raw_nmea_msg : NMEAMessage
        raw_rtcm_msg : RTCMMessage
        try:
            raw_nmea_msg, parsed_nmea_msg = self.nmea_reader.read()
            raw_rtcm_msg, parsed_rtcm_msg = self.rtcm_reader.read()
        except Exception as e:
            self.get_logger().warn(f'Failed to read NMEA sentence: {e}')
            return

        self.get_logger().debug(f'raw NMEA message: {raw_nmea_msg}')
        self.get_logger().debug(f'parsed NMEA message: {parsed_nmea_msg}')
        self.get_logger().debug(f'raw RTCM message: {raw_rtcm_msg}')
        self.get_logger().debug(f'parsed RTCM message: {parsed_rtcm_msg}, type: {type(parsed_rtcm_msg)}')

        if parsed_nmea_msg is None:
            return

        if GPS_MODULE == 'ublox':
            # wills code for the ublox module (16/04/25)
            parsed_nmea_str = str(parsed_nmea_msg)
            if 'lat=' in parsed_nmea_str:
                print('\nUblox GPS Module Data:')
                print('\traw=', parsed_nmea_str)
                match_lat = re.search(r'lat=([-\d.]+)', parsed_nmea_str)
                match_lon = re.search(r'lon=([-\d.]+)', parsed_nmea_str)
                latitude = 0
                longtitude = 0

                if match_lat:
                    latitude = float(match_lat.group(1))
                    print('\tlatitude=', latitude)
                
                if match_lon:
                    longtitude = float(match_lon.group(1))
                    print('\tlongitude=', longtitude)  

                if match_lat or match_lon:
                    self.pose.valid = True
                    self.pose.latitude, self.pose.longitude = latitude, longtitude
                    self.pose.heading_valid = False # Not RTK mode. We don't have valid heading
                    self.nmea_publisher.publish(self.pose)
                else: 
                    print('\tGPS data not available...')
                    self.pose.valid = False

            raw_rtcm_str = UInt8MultiArray()
            raw_rtcm_str.data = list(raw_rtcm_msg)
            self.rtcm_publisher.publish(raw_rtcm_str)
            

        # elif GPS_MODULE == 'skytraq':
        #     try:
        #         if parsed_nmea_msg.talker == 'P' and parsed_nmea_msg.msgID == 'STI' and parsed_nmea_msg.msgId == '036':
        #             # We are dealing with a PSTI036 message, which contains orientation information
        #             if parsed_nmea_msg.mode == 'R':
        #                 # RTK (Real-Time Kinematic) mode. We have valid heading
        #                 self.pose.heading_valid = True
        #                 self.pose.pitch, self.pose.roll, self.pose.yaw = parsed_nmea_msg.pitch, parsed_nmea_msg.roll, parsed_nmea_msg.heading
        #             else:
        #                 # Not RTK mode. We don't have valid heading
        #                 self.pose.heading_valid = False

        #         elif parsed_nmea_msg.talker == 'GN' and parsed_nmea_msg.msgID == 'RMC':
        #             if parsed_nmea_msg.status == 'A':
        #                 # Valid
        #                 self.pose.valid = True
        #                 self.pose.latitude, self.pose.longitude = parsed_nmea_msg.lat, parsed_nmea_msg.lon
        #                 self.nmea_publisher.publish(self.pose)
        #             else:
        #                 self.pose.valid = False

        #         elif parsed_nmea_msg.talker == 'GP' and parsed_nmea_msg.msgID == 'GGA':
        #             self.fix_type = parsed_nmea_msg.quality   # 1 = No fix, 2 = 2D fix, 3 = 3D fix
        #     except Exception as e:
        #         self.get_logger().warn(f'Bad message {parsed_nmea_msg}')

    def config_port(self, port_name, baud):
        self.ser.baudrate = baud
        if port_name == '':
            port_name = '/dev/ttyACM0'
        self.ser.port = port_name
        self.ser.open()

    def print_msg(self):
        roverMsgStr = f'''
        valid: {self.pose.valid}
        fix type: {'None' if self.fix_type == 1 else '2D' if self.fix_type == 2 else '3D' if self.fix_type == 3 else self.fix_type}
        lat: {self.pose.latitude:8.3f}
        lon: {self.pose.longitude:8.3f}
        pitch: {self.pose.pitch:8.2f}
        roll: {self.pose.roll:8.2f}
        yaw: {self.pose.yaw:8.2f}
        '''

        if self.pose.valid:
            self.get_logger().debug(roverMsgStr,throttle_duration_sec=2)
        else:
            self.get_logger().warn(f'{roverMsgStr}',throttle_duration_sec=2)

    def publisher_callback(self):
        self.parse_msg()
        self.print_msg()

        
def main (args = None):
    rclpy.init(args = args)
    node = GPSBase('', 115200)
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()
    
if __name__ == '__main__':
    main()