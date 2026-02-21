#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Reads RTCM3 error correction data from 
base (ublox) GPS and publishes to rover (skytraq) 
GPS.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gps_base
TOPICS:
 - publisher: /gps_base/fix    [NavSatFix]
 - publisher: /gps_base/rtcm   [UInt8MultiArray]
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
from pynmeagps import NMEAReader, NMEAMessage
from pyrtcm import RTCMReader, RTCMMessage
from pyubx2 import UBXReader, UBXMessage, val2sphp
import re
import rclpy
from rclpy.node import Node
from std_msgs.msg import UInt8MultiArray
from sensor_msgs.msg import NavSatFix
from rclpy.qos import QoSPresetProfiles
import logging
from rcl_interfaces.msg import ParameterDescriptor, ParameterType


class GPSBase(Node):
    def __init__ (self):
        super().__init__('gps_base')

        self.create_parameters()

        self.open_serial()

        self.create_publishers()

        self.config_rtcm(
            port_type=self.port_type, 
        )
        if (self.svin):
            self.get_logger().info(f'🛰️ Surveying position for >{self.min_dur}secs until <{self.acc_limit}mm accuracy achieved for RTK...', throttle_duration_sec=2)
            self.config_svin_rtk(
                port_type=self.port_type,
                acc_limit=self.acc_limit,
                min_dur=self.min_dur, 
            )
        else:
            self.get_logger().info(f'🛰️ Using position at {self.lat}, {self.lon} for RTK.', throttle_duration_sec=2)
            self.config_fixed_rtk(
                acc_limit=self.acc_limit, 
                lat=self.lat,
                lon=self.lon,
                height=self.height, 
            )

    def create_parameters(self) -> None:

        self.acc_limit = self.declare_parameter(
            name='acc_limit',
            value=100, 
            descriptor=ParameterDescriptor(
                type=ParameterType.PARAMETER_INTEGER,
                description='The minimum accuracy (in mm) RTK will attempt to achieve.'
            ),
        ).value

        self.baudrate = self.declare_parameter(
            name='baudrate', 
            value=115200, 
            descriptor=ParameterDescriptor(
                type=ParameterType.PARAMETER_INTEGER,
                description='The baudrate of the serial port being used.'
            ),
        ).value

        self.height = self.declare_parameter(
            name='height', # Height in cm
            value=9936.48, 
            descriptor=ParameterDescriptor(
                type=ParameterType.PARAMETER_DOUBLE,
                description='The height of the GPS module, if using fixed RTK (svin=False).'
            ),
        ).value

        self.lat = self.declare_parameter(
            name='lat', 
            value=-37.9098296, 
            descriptor=ParameterDescriptor(
                type=ParameterType.PARAMETER_DOUBLE,
                description='The latitude of the GPS module, if using fixed RTK (svin=False).'
            ),
        ).value

        self.lon = self.declare_parameter(
            name='lon', 
            value=145.1340534, 
            descriptor=ParameterDescriptor(
                type=ParameterType.PARAMETER_DOUBLE,
                description='The longitude of the GPS module, if using fixed RTK (svin=False).'
            ),
        ).value

        self.min_dur = self.declare_parameter(
            name='min_dur', # Seconds
            value=90, 
            descriptor=ParameterDescriptor(
                type=ParameterType.PARAMETER_INTEGER,
                description='The minimum amount of time (in seconds) RTK will survey-in, if using Survey-In RTK (svin=True).'
            ),
        ).value

        self.port_name = self.declare_parameter(
            name='port_name', 
            value='/dev/ttyACM0', 
            descriptor=ParameterDescriptor(
                type=ParameterType.PARAMETER_STRING,
                description='The name of the serial port being used.'
            ),
        ).value

        self.port_type = self.declare_parameter(
            name='port_type', # Choose from "USB", "UART1", "UART2"
            value='USB', 
            descriptor=ParameterDescriptor(
                type=ParameterType.PARAMETER_STRING,
                description='The type of the serial port being used.'
            ),
        ).value

        self.svin = self.declare_parameter(
            name='svin', # True = Survey-In, False = Fixed
            value=False, 
            descriptor=ParameterDescriptor(
                type=ParameterType.PARAMETER_BOOL,
                description='If true, use Survey-In RTK. Else, use Fixed RTK.'
            ),
        ).value

        self.fix_type = None

    def open_serial(self) -> None:
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

    def create_publishers(self) -> None:
        self.pub_pose = self.create_publisher(
            NavSatFix, 
            '/gps_base/fix', 
            QoSPresetProfiles.SENSOR_DATA.value, 
        )
        self.pub_rtcm = self.create_publisher(
            UInt8MultiArray, 
            '/gps_base/rtcm', 
            QoSPresetProfiles.SENSOR_DATA.value, 
        )
        self.pose = NavSatFix()
        self.pose.header.frame_id = 'gps_base'
        self.timer = self.create_timer(0, self.loop)

    def config_rtcm(self, port_type : str) -> None:
        '''
        Configure which RTCM3 messages to output and write to serial.
        '''
        self.get_logger().debug('Formatting RTCM MSGOUT CFG-VALSET message...', throttle_duration_sec=2)
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
        self.ser.write(msg_ubx.serialize())

        ### LOG ###
        self.get_logger().debug(f'Set RTCM3 MSGOUT Basestation,', throttle_duration_sec=2)
        self.get_logger().debug(f'CFG, CFG_VALSET, {msg_ubx.payload.hex()}, 1', throttle_duration_sec=2)

    def config_fixed_rtk(self, acc_limit : int, lat : float, lon : float, height : float) -> None:
        '''
        Configure Fixed mode with specified coordinates.
        '''
        self.get_logger().debug('Formatting FIXED TMODE CFG-VALSET message...', throttle_duration_sec=2)
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
        self.ser.write(msg_ubx.serialize())

        ### LOG ###
        self.get_logger().debug(f'CFG, CFG_VALSET, {msg_ubx.payload.hex()}, 1', throttle_duration_sec=2)

    def config_svin_rtk(self, port_type : str, acc_limit : int, min_dur : int) -> None:
        '''
        Configure Survey-In mode with specied accuracy limit.
        '''
        self.get_logger().debug('Formatting SVIN TMODE CFG-VALSET message...', throttle_duration_sec=2)
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
        self.ser.write(msg_ubx.serialize())

        ### LOG ###
        self.get_logger().debug(f'CFG, CFG_VALSET, {msg_ubx.payload.hex()}, 1', throttle_duration_sec=2)

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
                self.pose.status.status = 0
                self.pose.status.service = 0
                self.pose.position_covariance = [
                    0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0,
                    0.0, 0.0, 0.0
                ]
                self.pose.position_covariance_type = 0
                self.pose.latitude, self.pose.longitude = latitude, longtitude
            else: 
                self.get_logger().warn(f'❌ GPS data is not available!', throttle_duration_sec=2)

            ### ROS2 ###
            self.pub_pose.publish(self.pose)
            
            ### LOG ###
            msg_log = f'''
                🛰️ NMEA Data:
                \traw: {msg_str}
                \tlat: {self.pose.latitude:8.3f}
                \tlon: {self.pose.longitude:8.3f}
            '''

    def parse_rtcm(self) -> None:
        self.pose.header.stamp = self.get_clock().now().to_msg()
        msg_raw : RTCMMessage
        try:
            msg_raw, msg_parsed = self.reader_rtcm.read()
        except Exception as e:
            self.get_logger().warn(f'❌ Failed to read RTCM3 message: {e}', throttle_duration_sec=2)
            return

        self.get_logger().debug(f'✅ RTCM3 message received!', throttle_duration_sec=2)
        self.get_logger().debug(f'\t   raw RTCM3 message: {msg_raw}', throttle_duration_sec=2)
        self.get_logger().debug(f'\tparsed RTCM3 message: {msg_parsed}', throttle_duration_sec=2)

        if msg_parsed is None:
            self.get_logger().warn(f'❌ Failed to read RTCM3 message: \'msg_parsed\' cannot be None!', throttle_duration_sec=2)
            return

        msg_str = str(msg_parsed)

        ### ROS2 ###
        msg_binary = UInt8MultiArray()
        msg_binary.data = list(msg_raw)
        self.pub_rtcm.publish(msg_binary)

        ### LOG ###
        msg_log = f'''
            🛰️ RTCM3 Data:
            \traw: {msg_str}
        '''
        if self.fix_type == 3:
            self.get_logger().debug(msg_log, throttle_duration_sec=2)
        else:
            self.get_logger().warn(msg_log, throttle_duration_sec=2)

    def loop(self) -> None:
        self.parse_nmea()
        self.parse_rtcm()

        
def main (args = None):
    rclpy.init(args = args)
    node = GPSBase()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()
    
if __name__ == '__main__':
    main()