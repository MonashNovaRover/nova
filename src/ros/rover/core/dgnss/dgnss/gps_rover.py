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
import rclpy
from rclpy.node import Node
from rclpy.qos import QoSPresetProfiles
from std_msgs.msg import UInt8MultiArray, Float64
from sensor_msgs.msg import NavSatFix, NavSatStatus
from nova_interfaces.msg import GPSData
from serial import Serial
from pyunigps import (
    UNIReader,
    NMEA_PROTOCOL,
)
import threading
import time

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
            # UM982
            value='/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0', 
        ).value
        self.publisher_rate = self.declare_parameter(
            name='publisher_rate', 
            value=20, 
        ).value
        self.fix_type = None

        ### Serial ###
        self.ser = Serial()
        self.config_port(self.port_name, self.baudrate)
        self.reader = UNIReader(
            self.ser, 
            protfilter=NMEA_PROTOCOL,
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
            self.sub_magnetometer_callback, 
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
        self.valid_gps_heading = False
        
        ### Threading ###
        self.fix_lock = threading.Lock()
        self.serial_lock = threading.Lock()
        self.running = True
        self.reader_thread = threading.Thread(target=self.reader_loop, daemon=True)
        self.reader_thread.start()

        self.get_logger().debug(f'Node configured!')

    def config_port(self, port_name : str, baudrate : int):
        self.ser.baudrate = baudrate
        self.ser.port = port_name
        self.ser.timeout = 3.0
        self.ser.open()

        self.get_logger().debug(f'Serial port configured!')

    def sub_rtcm_callback(self, msg : UInt8MultiArray):
        msg_binary = bytes(msg.data)
        
        try:
            with self.serial_lock:
                self.ser.write(msg_binary)
        except Exception as e:
            self.get_logger().warn(f'❌ Failed to write RTCM to serial port: {e}')

    def sub_magnetometer_callback(self, msg : Float64):
        if not self.valid_gps_heading:
            # use mag heading as backup
            self.pose_custom.heading = msg.data

    def reader_loop(self):
        while self.running and rclpy.ok():
            if self.ser.in_waiting == 0:
                time.sleep(0.005)  # Avoid busy waiting
                continue

            try:
                with self.serial_lock:
                    _, msg_parsed = self.reader.read()

                if msg_parsed is not None:
                    with self.fix_lock:
                        self.process_nmea(msg_parsed)
            
            except Exception as e:
                self.get_logger().warn(f'❌ Failed to read NMEA message: {e}')
                self.get_logger().info(f'Reopening serial port...')
                self.ser.close()
                self.ser.open()
                time.sleep(0.01)

    def process_nmea(self, msg_parsed: str):
        if msg_parsed is None:
            self.get_logger().warn(f'❌ Failed to read NMEA message: \'msg_parsed\' cannot be None!')
            return

        if msg_parsed.msgID == 'GGA':
            self.get_logger().info(f'NMEA GGA message quality: {msg_parsed.quality}', throttle_duration_sec=1)
            if msg_parsed.quality > 0:
                # Valid fix
                self.pose.latitude = float(msg_parsed.lat)
                self.pose.longitude = float(msg_parsed.lon)
                self.pose.altitude = float(msg_parsed.alt)
                self.pose.status.status = NavSatStatus.STATUS_FIX
                self.fix_type = 'GPS fix'
                if msg_parsed.quality == 4:
                    self.pose.status.status = NavSatStatus.STATUS_GBAS_FIX
                    self.fix_type = 'RTK fixed'
                elif msg_parsed.quality == 5:
                    self.pose.status.status = NavSatStatus.STATUS_GBAS_FIX
                    self.fix_type = 'RTK float'
            else:
                self.pose.status.status = NavSatStatus.STATUS_NO_FIX
                self.fix_type = 'No fix'
                self.get_logger().warn(f'❌ GNSS (GGA) data is not available!', throttle_duration_sec=1)
        elif msg_parsed.msgID == 'THS':
            if msg_parsed.mi != 'V':
                self.pose_custom.heading = float(msg_parsed.headt)
                self.valid_gps_heading = True
            else:
                self.get_logger().warn(f'❌ GNSS (THS) heading data is invalid!', throttle_duration_sec=1)
                self.valid_gps_heading = False

        ### LOG ###
        msg_log = f'''
            {'-'*30}
            🛰️ NMEA Data:
            \tlat: {self.pose.latitude:8.7f}
            \tlon: {self.pose.longitude:8.7f}
            \talt: {self.pose.altitude:8.7f}
            \theading: {self.pose_custom.heading:8.3f}
            \tfix type: {self.fix_type}
            {'-'*30}
        '''
        self.get_logger().info(msg_log, throttle_duration_sec=1)
        self.get_logger().debug(f'NMEA message parsed!')

        # Copy data to custom message
        self.pose_custom.header = self.pose.header
        self.pose_custom.status = self.pose.status
        self.pose_custom.latitude = self.pose.latitude
        self.pose_custom.longitude = self.pose.longitude
        self.pose_custom.altitude = self.pose.altitude

    def loop(self) -> None:
        with self.fix_lock:
            self.pose.header.stamp = self.get_clock().now().to_msg()
            self.pub_pose.publish(self.pose)
            self.pub_pose_custom.publish(self.pose_custom)

    def destroy_node(self):
        self.running = False
        if hasattr(self, 'reader_thread'):
            self.reader_thread.join(timeout=1.0)
        self.ser.close()
        super().destroy_node()


def main (args = None):
    rclpy.init(args = args)
    node = GPSRover()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
