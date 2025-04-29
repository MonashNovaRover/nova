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
SERVICES: None
ACTIONS: None
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
        self.acc_limit = self.declare_parameter('acc_limit', 1000).value                            # accuracy in mm
        self.baudrate = self.declare_parameter('baudrate', 115200).value
        self.gps_module = self.declare_parameter('gps_module', 'ublox').value
        self.height = self.declare_parameter('height', 0.).value
        self.lat = self.declare_parameter('lat', 0.).value
        self.lon = self.declare_parameter('lon', 0.).value
        self.max_calibration_error = self.declare_parameter('max_calibration_error', 5e-5).value    # degrees
        self.min_dur = self.declare_parameter('min_dur', 60).value                                  # seconds
        self.port_name = self.declare_parameter('port_name', '/dev/ttyACM0').value
        self.port_type = self.declare_parameter('port_type', 'USB').value                           # choose from "USB", "UART1", "UART2"
        self.svin = self.declare_parameter('svin', 'True').value                                    # True = Survey-In, False = Fixed

        self.pose = RoverPoseGPS()
        self.pose.header.frame_id = 'gps_base'

        self.fix_type = None

        ### Serial ###
        self.ser = Serial()
        self.config_port(self.port_name, self.baudrate)
        self.read_nmea = NMEAReader(
            self.ser, 
            validate=0x03,  # validate both checksum and message id
            nmeaonly=True,  # Raise an error on receiving a badly formatted message
        )
        self.read_rtcm = RTCMReader(
            self.ser, 
            validate=0x01,  # validate checksum
        )
        self.read_ubx = UBXReader(
            self.ser, 
            validate=0x01,  # validate checksum
        )

        ### ROS2 ###
        self.pub_nmea = self.create_publisher(
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

        ### INITIALISE ###
        self.config_rtcm(
            port_type=PORT_TYPE, 
        )
        if (self.svin.lower() == 'true'):
            self.config_svin(
                port_type=PORT_TYPE, 
                acc_limit=ACC_LIMIT, 
                min_dur=SVIN_MIN_DUR, 
            )
        else:
            self.config_fixed(
                acc_limit=ACC_LIMIT, 
                lat=0, 
                lon=0, 
                height=0, 
            )

        self.get_logger().info('gps_base started.')

    def config_rtcm(port_type : str) -> None:
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

        if SHOW_PRESET:
            print(
                'Set ZED-F9P RTCM3 MSGOUT Basestation, '
                f'CFG, CFG_VALSET, {msg_ubx.payload.hex()}, 1\n'
            )

        self.ser.write(msg_ubx.serialize())

    def config_fixed(acc_limit : int, lat : float, lon : float, height : float) -> None:
        '''
        Configure Fixed mode with specified coordinates.
        '''

        print('\nFormatting FIXED TMODE CFG-VALSET message...')
        tmode = TMODE_FIXED
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

        if SHOW_PRESET:
            print(
                'Set ZED-F9P to Fixed Timing Mode Basestation, '
                f'CFG, CFG_VALSET, {msg_ubx.payload.hex()}, 1\n'
            )

        self.ser.write(msg_ubx.serialize())

    def config_svin(port_type : str, acc_limit : int, min_dur : int) -> UBXMessage:
        '''
        Configure Survey-In mode with specied accuracy limit.
        '''

        print('\nFormatting SVIN TMODE CFG-VALSET message...')
        tmode = TMODE_SVIN
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

        if SHOW_PRESET:
            print(
                'Set ZED-F9P to Survey-In Timing Mode Basestation, '
                f'CFG, CFG_VALSET, {msg_ubx.payload.hex()}, 1\n'
            )

        self.ser.write(msg_ubx.serialize())

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

    def config_port(self, port_name : str, baudrate : int) -> None:
        self.ser.baudrate = baudrate
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

    def loop(self):
        self.parse_msg()
        self.print_msg()

        
def main (args = None):
    rclpy.init(args = args)
    node = GPSBase()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()
    
if __name__ == '__main__':
    main()