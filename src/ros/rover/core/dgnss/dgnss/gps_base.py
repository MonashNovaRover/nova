#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Reads RTCM3 error correction data from 
base (ublox) GPS and publishes to rover (skytraq) 
GPS.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gps_base
TOPICS:
 - publisher: /gps_base/fix         [NavSatFix]
 - publisher: /gps_base/fix_custom  [GPSData]
 - publisher: /gps_base/rtcm        [UInt8MultiArray]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Shelby N, Will Middlewick, Victor 
            Bartlinski
CREATED:	25/02/2023
EDITED:		30/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Better document the message protocols being 
   sent in config_*_rtk functions. 
 - Implement fix_type. 
 - Abstract serial protocol parsing to a function 
   for each protocol; only UBX messages under the 
   'ublox' module condition. 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from serial import Serial
from pynmeagps import NMEAReader
from pyrtcm import RTCMReader, RTCMMessage
from pyubx2 import UBXMessage, val2sphp
import re
import rclpy
from rclpy.node import Node
from std_msgs.msg import UInt8MultiArray
from sensor_msgs.msg import NavSatFix, NavSatStatus
from nova_interfaces.msg import GPSData
from rclpy.qos import QoSPresetProfiles


class GPSBase(Node):
    def __init__ (self):
        super().__init__('gps_base')
        self.baudrate = self.declare_parameter(
            name='baudrate', 
            value=115200, 
        ).value
        self.port_name = self.declare_parameter(
            name='port_name', 
            # UM960
            value='/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0', 
        ).value
        self.port_type = self.declare_parameter(
            name='port_type', # Choose from "USB", "UART1", "UART2"
            value='USB', 
        ).value
        self.svin = self.declare_parameter(
            name='svin', # True = Survey-In, False = Fixed
            value=False, 
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
        self.reader_rtcm = RTCMReader(
            self.ser,
            parsed=False,
        )

        ### ROS2 ###
        self.pub_pose = self.create_publisher(
            NavSatFix, 
            '/gps_base/fix', 
            QoSPresetProfiles.SENSOR_DATA.value, 
        )
        self.pub_pose_custom = self.create_publisher(
            GPSData,
            '/gps_base/fix_custom',
            QoSPresetProfiles.SENSOR_DATA.value,
        )
        self.pub_rtcm = self.create_publisher(
            UInt8MultiArray, 
            '/gps_base/rtcm', 
            QoSPresetProfiles.SENSOR_DATA.value, 
        )
        self.pose = NavSatFix()
        self.pose_custom = GPSData()
        self.pose.header.frame_id = 'gps_base'
        self.timer = self.create_timer(0, self.loop)

    def config_port(self, port_name : str, baudrate : int) -> None:
        self.ser.baudrate = baudrate
        if port_name == '':
            port_name = '/dev/ttyACM0'
        self.ser.port = port_name
        self.ser.open()

    def parse_nmea(self) -> None:
        self.pose.header.stamp = self.get_clock().now().to_msg()
        try:
            msg_raw, msg_parsed = self.reader_nmea.read()
        except Exception as e:
            self.get_logger().warn(f'❌ Failed to read NMEA message: {e}', throttle_duration_sec=2)
            return

        self.get_logger().debug(f'✅ NMEA message received!', throttle_duration_sec=2)
        self.get_logger().debug(f'\t   raw NMEA message: {msg_raw}', throttle_duration_sec=2)
        self.get_logger().debug(f'\tparsed NMEA message: {msg_parsed}', throttle_duration_sec=2)

        if msg_parsed is None:
            self.get_logger().warn(f'❌ Failed to read NMEA message: \'msg_parsed\' cannot be None!', throttle_duration_sec=2)
            return

        if msg_parsed.msgID == 'GGA':
            if msg_parsed.quality > 0:
                # Valid fix
                self.pose.latitude = float(msg_parsed.lat)
                self.pose.longitude = float(msg_parsed.lon)
                self.pose.altitude = float(msg_parsed.alt)
                self.pose.status.status = NavSatStatus.STATUS_FIX
                self.fix_type = 'GPS fix'
            else:
                self.pose.status.status = NavSatStatus.STATUS_NO_FIX
                self.fix_type = 'No fix'
                self.get_logger().warn(f'❌ GPS (GGA) data is not available!', throttle_duration_sec=2)

            ### ROS2 ###
            self.pose_custom.header = self.pose.header
            self.pose_custom.status = self.pose.status
            self.pose_custom.latitude = self.pose.latitude
            self.pose_custom.longitude = self.pose.longitude
            self.pose_custom.altitude = self.pose.altitude

            ### LOG ###
            msg_log = f'''
                🛰️ NMEA Data:
                \tlat: {self.pose.latitude:8.3f}
                \tlon: {self.pose.longitude:8.3f}
                \talt: {self.pose.altitude:8.3f}
                \tfix type: {self.fix_type}
            '''
            self.get_logger().info(msg_log, throttle_duration_sec=1)
            

    def parse_rtcm(self) -> None:
        self.pose.header.stamp = self.get_clock().now().to_msg()
        try:
            msg_raw, _ = self.reader_rtcm.read()
        except Exception as e:
            self.get_logger().warn(f'❌ Failed to read RTCM3 message: {e}', throttle_duration_sec=1)
            return

        self.get_logger().info(f'✅ RTCM3 message received!', throttle_duration_sec=1)
        self.get_logger().info(f'\t   raw RTCM3 message: {msg_raw}', throttle_duration_sec=1)

        ### ROS2 ###
        msg_binary = UInt8MultiArray()
        msg_binary.data = list(msg_raw)
        self.pub_rtcm.publish(msg_binary)
        self.get_logger().info(f'✅ Published RTCM3 message!', throttle_duration_sec=1)

    def loop(self) -> None:
        self.parse_nmea()
        self.parse_rtcm()
        self.pub_pose.publish(self.pose)
        self.pub_pose_custom.publish(self.pose_custom)

        
def main (args = None):
    rclpy.init(args = args)
    node = GPSBase()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()
    
if __name__ == '__main__':
    main()