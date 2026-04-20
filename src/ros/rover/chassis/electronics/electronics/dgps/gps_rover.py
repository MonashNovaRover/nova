#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Takes RTCM error correction data from 
base (ublox) GPS and writes data to the rover 
(skytraq) GPS over USB.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gps_rover
TOPICS:
  - subscriber: /gps_base/rtcm        [UInt8MultiArray]
  - publisher: /gps_rover/fix         [NavSatFix]
  - publisher: /gps_rover/fix_custom  [GPSData]
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
from pynmeagps import NMEAReader
import re
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSPresetProfiles
from std_msgs.msg import UInt8MultiArray, Float64
from sensor_msgs.msg import NavSatFix
from nova_interfaces.msg import GPSData

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
            value='/dev/serial/by-id/usb-Silicon_Labs_CP2102_USB_to_UART_Bridge_Controller_0001-if00-port0', 
        ).value
        self.gps_module = self.declare_parameter(
            name='gps_module', 
            value='skytraq', 
        ).value
        self.publisher_rate = self.declare_parameter(
            name='publisher_rate', 
            value=30, 
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
            QoSPresetProfiles.SENSOR_DATA.value, 
        )
        self.sub_heading = self.create_subscription(
            Float64, 
            'mag/heading', 
            self.sub_heading_callback, 
            QoSPresetProfiles.SENSOR_DATA.value, 
        )
        self.pub_pose = self.create_publisher(
            NavSatFix, 
            '/gps_rover/fix', 
            QoSPresetProfiles.SENSOR_DATA.value, 
        )
        self.pub_pose_custom = self.create_publisher(
            GPSData, 
            '/gps_rover/fix_custom', 
            QoSPresetProfiles.SENSOR_DATA.value, 
        )
        self.pose = NavSatFix()
        self.pose_custom = GPSData()
        self.pose.header.frame_id = 'gps'
        self.timer = self.create_timer(1/self.publisher_rate, self.loop)

        self.get_logger().debug(f'Node configured!')

    def config_port(self, port_name : str, baudrate : int):
        self.get_logger().debug(f'Configuring serial port...')

        self.ser.baudrate = baudrate
        if port_name == '':
           port_name = '/dev/ttyUSB0'
        self.ser.port = port_name
        self.ser.open()

        self.get_logger().debug(f'Serial port configured!')

    def sub_rtcm_callback(self, msg : UInt8MultiArray):
        msg_binary = bytes(msg.data)
        self.ser.write(msg_binary)

    def sub_heading_callback(self, msg : Float64):
        self.pose_custom.heading = msg.data

    def parse_nmea(self) -> None:
        self.get_logger().debug(f'Parsing NMEA message...')
        try: 
            self.pose.header.stamp = self.get_clock().now().to_msg()
            try:
                msg_raw, msg_parsed = self.reader_nmea.read()
            except Exception as e:
                self.get_logger().warn(f'❌ Failed to read NMEA message: {e}')
                return

            if msg_parsed is None:
                self.get_logger().warn(f'❌ Failed to read NMEA message: \'msg_parsed\' cannot be None!')
                return

            self.get_logger().debug(f'✅ NMEA message received!')
            self.get_logger().debug(f'\t   raw NMEA message: {msg_raw}')
            self.get_logger().debug(f'\tparsed NMEA message: {msg_parsed}')

            if self.gps_module == 'skytraq':
                try:
                    msg_str = str(msg_parsed)
                    if msg_parsed.talker == 'GP' and msg_parsed.msgID == 'GGA':
                        if msg_parsed.quality > 0:
                            # Valid fix
                            self.pose.latitude = float(msg_parsed.lat)
                            self.pose.longitude = float(msg_parsed.lon)
                            self.pose.altitude = float(msg_parsed.alt)
                        else:
                            self.get_logger().warn(f'❌ GPS (GGA) data is not available!', throttle_duration_sec=2)
                    if msg_parsed.talker == 'P' and msg_parsed.msgID == 'STI' and msg_parsed.msgId == '036':
                        # We are dealing with a PSTI036 message, which contains orientation information
                        if msg_parsed.mode == 'R':
                            # RTK (Real-Time Kinematic) mode. We have valid heading
                            self.pose.status.status = 0
                            self.pose.status.service = 0
                            self.pose.position_covariance = [
                                0.0, 0.0, 0.0,
                                0.0, 0.0, 0.0,
                                0.0, 0.0, 0.0
                            ]
                            self.pose.position_covariance_type = 0
                    elif msg_parsed.talker == 'GN' and msg_parsed.msgID == 'RMC':
                        if msg_parsed.status == 'A':
                            # Valid
                            self.pose.status.status = 0
                            self.pose.status.service = 0
                            self.pose.position_covariance = [
                                0.0, 0.0, 0.0,
                                0.0, 0.0, 0.0,
                                0.0, 0.0, 0.0
                            ]
                            self.pose.position_covariance_type = 0
                            self.pose.latitude, self.pose.longitude = msg_parsed.lat, msg_parsed.lon

                    ### LOG ###
                    msg_log = f'''
                        🛰️ NMEA Data:
                        \traw: {msg_str}
                        \tlat: {self.pose.latitude:8.3f}
                        \tlon: {self.pose.longitude:8.3f}
                        \talt: {self.pose.altitude:8.3f}
                    '''

                except Exception as e:
                    self.get_logger().warn(f'❌ Error: {e}, Bad message: {msg_parsed}')
        except Exception as e:
            self.get_logger().warn(f'❌ Unknown error: {e}')
            return
        self.get_logger().debug(f'NMEA message parsed!')

        # Copy data to custom message
        self.pose_custom.header = self.pose.header
        self.pose_custom.status = self.pose.status
        self.pose_custom.latitude = self.pose.latitude
        self.pose_custom.longitude = self.pose.longitude
        self.pose_custom.altitude = self.pose.altitude

    def loop(self) -> None:
        self.parse_nmea()
        self.pub_pose.publish(self.pose)
        self.pub_pose_custom.publish(self.pose_custom)

def main (args = None):
    rclpy.init(args = args)
    node = GPSRover()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
