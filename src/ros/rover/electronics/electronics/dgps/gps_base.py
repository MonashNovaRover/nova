#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Reads RTCM3 error correction data from 
base (ublox) GPS and publishes to rover (skytraq) 
GPS.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gps_base
TOPICS:
  - publisher: /gps_base/fix    [RoverPoseGPS]
  - publisher: /gps_base/rtcm   [UInt8MultiArray]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Shelby N, Will Middlewick, Victor 
            Bartlinski
CREATED:	25/02/2023
EDITED:		30/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - convert log to debug
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
from serial import Serial
from pynmeagps import NMEAReader, NMEAMessage
from pyrtcm import RTCMReader, RTCMMessage
from pyubx2 import UBXReader, UBXMessage, val2sphp
import re
import rclpy
from rclpy.node import Node
from std_msgs.msg import UInt8MultiArray

from nova_interfaces.msg import RoverPoseGPS
import logging


class GPSBase(Node):
    def __init__ (self):
        super().__init__('gps_base')
        self.acc_limit = self.declare_parameter('acc_limit', 100).value                             # accuracy in mm
        self.baudrate = self.declare_parameter('baudrate', 115200).value
        self.gps_module = self.declare_parameter('gps_module', 'ublox').value
        self.height = self.declare_parameter('height', 9936.48).value                                    # cm
        self.lat = self.declare_parameter('lat', -37.9098296).value
        self.lon = self.declare_parameter('lon', 145.1340534).value
        self.max_calibration_error = self.declare_parameter('max_calibration_error', 5e-5).value    # degrees
        self.min_dur = self.declare_parameter('min_dur', 90).value                                  # seconds
        self.port_name = self.declare_parameter('port_name', '/dev/ttyACM0').value
        self.port_type = self.declare_parameter('port_type', 'USB').value                           # choose from "USB", "UART1", "UART2"
        self.svin = self.declare_parameter('svin', 'False').value                                   # True = Survey-In, False = Fixed
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
            validate=0x01,  # validate checksum
        )

        ### ROS2 ###
        self.pub_pose = self.create_publisher(
            RoverPoseGPS, 
            '/gps_base/fix', 
            10, 
        )
        self.pub_rtcm = self.create_publisher(
            UInt8MultiArray, 
            '/gps_base/rtcm', 
            10, 
        )
        self.timer = self.create_timer(0, self.loop)
        self.pose = RoverPoseGPS()
        self.pose.header.frame_id = 'gps_base'

        ### INITIALISE ###
        self.get_logger().info(f'AAAAAAAAAAAAA{self.port_type, type(self.port_type)}')
        self.config_rtcm(
            port_type=self.port_type, 
        )
        if (self.svin.lower() == 'true'):
            self.get_logger().info(f'Using Survey-in RTK, .')
            self.config_svin_rtk(
                port_type=self.port_type,
                acc_limit=self.acc_limit,
                min_dur=self.min_dur, 
            )
        else:
            self.get_logger().info(f'Using Fixed RTK, lat={self.lat}, lon={self.lon}, height={self.height}.')
            self.config_fixed_rtk(
                acc_limit=self.acc_limit, 
                lat=self.lat,
                lon=self.lon,
                height=self.height, 
            )

        self.get_logger().info('gps_base started.')

    def config_rtcm(self, port_type : str) -> None:
        '''
        Configure which RTCM3 messages to output and write to serial.
        '''

        print('\nFormatting RTCM MSGOUT CFG-VALSET message...')
        layers = 1  # 1 = RAM, 2 = BBR, 4 = Flash (can be OR'd)
        transaction = 0
        cfg_data = []
        for rtcm_type in (
            '1005',
            '1077',
            '1087',
            '1097',
            '1127',
            '1230',
        ):
            cfg = f'CFG_MSGOUT_RTCM_3X_TYPE{rtcm_type}_{port_type}'
            cfg_data.append([cfg, 1])

        msg_ubx = UBXMessage.config_set(layers, transaction, cfg_data)

        ### LOG ###
        print(
            'Set ZED-F9P RTCM3 MSGOUT Basestation, '
            f'CFG, CFG_VALSET, {msg_ubx.payload.hex()}, 1\n'
        )

        self.ser.write(msg_ubx.serialize())

    def config_fixed_rtk(self, acc_limit : int, lat : float, lon : float, height : float) -> None:
        '''
        Configure Fixed mode with specified coordinates.
        '''

        print('\nFormatting FIXED TMODE CFG-VALSET message...')
        tmode = 2
        pos_type = 1  # LLH (as opposed to ECEF)
        layers = 1
        transaction = 0
        acc_limit = int(round(acc_limit / 0.1, 0))
        lats, lath = val2sphp(lat)
        lons, lonh = val2sphp(lon)

        height = int(height)
        cfg_data = [
            ('CFG_TMODE_MODE', tmode),
            ('CFG_TMODE_POS_TYPE', pos_type),
            ('CFG_TMODE_FIXED_POS_ACC', acc_limit),
            ('CFG_TMODE_HEIGHT_HP', 0),
            ('CFG_TMODE_HEIGHT', height),
            ('CFG_TMODE_LAT', lats),
            ('CFG_TMODE_LAT_HP', lath),
            ('CFG_TMODE_LON', lons),
            ('CFG_TMODE_LON_HP', lonh),
        ]

        msg_ubx = UBXMessage.config_set(layers, transaction, cfg_data)

        ### LOG ###
        print(
            'Set ZED-F9P to Fixed Timing Mode Basestation, '
            f'CFG, CFG_VALSET, {msg_ubx.payload.hex()}, 1\n'
        )

        self.ser.write(msg_ubx.serialize())

    def config_svin_rtk(self, port_type : str, acc_limit : int, min_dur : int) -> None:
        '''
        Configure Survey-In mode with specied accuracy limit.
        '''

        print('\nFormatting SVIN TMODE CFG-VALSET message...')
        tmode = 1
        layers = 1
        transaction = 0
        acc_limit = int(round(acc_limit / 0.1, 0))
        cfg_data = [
            ('CFG_TMODE_MODE', tmode),
            ('CFG_TMODE_SVIN_ACC_LIMIT', acc_limit),
            ('CFG_TMODE_SVIN_MIN_DUR', min_dur),
            (f'CFG_MSGOUT_UBX_NAV_SVIN_{port_type}', 1),
        ]

        msg_ubx = UBXMessage.config_set(layers, transaction, cfg_data)

        ### LOG ###
        print(
            'Set ZED-F9P to Survey-In Timing Mode Basestation, '
            f'CFG, CFG_VALSET, {msg_ubx.payload.hex()}, 1\n'
        )

        self.ser.write(msg_ubx.serialize())

    def config_port(self, port_name : str, baudrate : int) -> None:
        self.ser.baudrate = baudrate
        if port_name == '':
            port_name = '/dev/ttyACM0'
        self.ser.port = port_name
        self.ser.open()

    def parse_nmea(self) -> None:
        self.pose.header.stamp = self.get_clock().now().to_msg()
        msg_raw : NMEAMessage
        try:
            msg_raw, msg_parsed = self.reader_nmea.read()
        except Exception as e:
            self.get_logger().warn(f'Failed to read NMEA message: {e}')
            return

        self.get_logger().debug(f'   raw NMEA message: {msg_raw}')
        self.get_logger().debug(f'parsed NMEA message: {msg_parsed}')

        if msg_parsed is None:
            self.get_logger().warn(f'Failed to read NMEA message: msg_parsed is None')
            return

        if self.gps_module == 'ublox':
            msg_str = str(msg_parsed)
            if 'lat=' in msg_str:
                match_lat = re.search(r'lat=([-\d.]+)', msg_str)
                match_lon = re.search(r'lon=([-\d.]+)', msg_str)
                latitude = 0
                longtitude = 0

                if match_lat:
                    latitude = float(match_lat.group(1))
                
                if match_lon:
                    longtitude = float(match_lon.group(1))

                if match_lat or match_lon:
                    self.pose.valid = True
                    self.pose.heading_valid = False # Not RTK mode. We don't have valid heading
                    self.pose.latitude, self.pose.longitude = latitude, longtitude
                else: 
                    self.pose.valid = False

                ### ROS2 ###
                self.pub_pose.publish(self.pose)
                
                ### LOG ###
                print('\nUblox GPS Module NMEA Data:')
                print(f'\traw={msg_str}')
                if match_lat: print(f'\tlatitude={latitude}')
                if match_lon: print(f'\tlongitude={longtitude}')
                if not (match_lat or match_lon): print('\tGPS data not available...')

    def parse_rtcm(self) -> None:
        self.pose.header.stamp = self.get_clock().now().to_msg()
        msg_raw : RTCMMessage
        try:
            msg_raw, msg_parsed = self.reader_rtcm.read()
        except Exception as e:
            self.get_logger().warn(f'Failed to read RTCM message: {e}')
            return

        self.get_logger().debug(f'   raw RTCM message: {msg_raw}')
        self.get_logger().debug(f'parsed RTCM message: {msg_parsed}')

        if msg_parsed is None:
            return

        if self.gps_module == 'ublox':
            msg_str = str(msg_parsed)

            ### ROS2 ###
            msg_binary = UInt8MultiArray()
            msg_binary.data = list(msg_raw)
            self.pub_rtcm.publish(msg_binary)

            ### LOG ###
            print('\nUblox GPS Module RTCM3 Data:')
            print(f'\traw={msg_str}')

    def log_msg(self) -> None:
        msg = f'''
        valid: {self.pose.valid}
        fix type: {'None' if self.fix_type == 1 else '2D' if self.fix_type == 2 else '3D' if self.fix_type == 3 else self.fix_type}
        lat: {self.pose.latitude:8.3f}
        lon: {self.pose.longitude:8.3f}
        pitch: {self.pose.pitch:8.2f}
        roll: {self.pose.roll:8.2f}
        yaw: {self.pose.yaw:8.2f}
        '''

        if self.pose.valid:
            self.get_logger().debug(msg,throttle_duration_sec=2)
        else:
            self.get_logger().warn(msg,throttle_duration_sec=2)

    def loop(self) -> None:
        self.parse_nmea()
        self.parse_rtcm()
        self.log_msg()

        
def main (args = None):
    rclpy.init(args = args)
    node = GPSBase()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()
    
if __name__ == '__main__':
    main()