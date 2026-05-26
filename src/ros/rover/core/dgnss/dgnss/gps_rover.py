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
EDITED:		26/05/2026
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
from pyubx2 import UBXReader
import threading
import time
import struct
from smbus2 import SMBus, i2c_msg
import time

class UbloxI2C:
    def __init__(self, bus_num=4, addr=0x42):
        self.bus_num = bus_num
        self.addr = addr
        self.bus = None

    def open(self):
        from smbus2 import SMBus
        self.bus = SMBus(self.bus_num)

    @property
    def in_waiting(self):
        """Mimics PySerial's in_waiting by checking u-blox registers 0xFD and 0xFE."""
        if not self.bus:
            return 0
        try:
            high = self.bus.read_byte_data(self.addr, 0xFD)
            low = self.bus.read_byte_data(self.addr, 0xFE)
            return (high << 8) | low
        except OSError:
            # I2C bus collision or device busy
            return 0

    def read(self, length):
        from smbus2 import i2c_msg
        import time
        if not self.bus:
            return b''
            
        data = bytearray()
        while len(data) < length:
            try:
                avail = self.in_waiting
                if avail > 0:
                    to_read = min(length - len(data), avail, 32)
                    
                    # Target the 0xFF Data Stream Register
                    write_reg = i2c_msg.write(self.addr, [0xFF])
                    self.bus.i2c_rdwr(write_reg)
                    
                    # Read the bytes
                    read_msg = i2c_msg.read(self.addr, to_read)
                    self.bus.i2c_rdwr(read_msg)
                    
                    data.extend(list(read_msg))
                else:
                    time.sleep(0.01) 
            except OSError:
                time.sleep(0.01)
                
        return bytes(data)

    def readline(self):
        """Mimics PySerial's readline() for NMEA string parsing."""
        line = bytearray()
        while True:
            # Only read if there is data, otherwise we might block too long
            if self.in_waiting > 0:
                char = self.read(1)
                line.extend(char)
                if char == b'\n':
                    break
            else:
                # If buffer is empty and we haven't hit a newline, break to avoid hanging
                break
        return bytes(line)

    def close(self):
        if self.bus:
            self.bus.close()

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
        self.protocol = self.declare_parameter(
            name='protocol',
            value='nmea'
        ).value

        ### Serial ###
        self.ser = Serial()
        self.config_port(self.port_name, self.baudrate)

        def sub_to_rtcm():
            self.sub_rtcm = self.create_subscription(
                UInt8MultiArray,
                'gps_base/rtcm',
                self.sub_rtcm_callback,
                QoSPresetProfiles.SENSOR_DATA.value,
            )

        if self.protocol == 'nmea':
            self.reader = UNIReader(
                self.ser,
                protfilter=NMEA_PROTOCOL,
            )
            sub_to_rtcm()
            self.reader_loop = self.nmea_loop
        elif self.protocol == 'ubx':
            self.reader_loop = self.ubx_loop
        else:
            raise ValueError(f'Unrecognised protocol selected: {self.protocol}')

        self.fix_type = None

        ### ROS2 ###
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
        self.ubx_reader = UBXReader(self.ser)

    def config_port(self, port_name, baudrate):
        if "i2c" in port_name:
            # Route to our custom I2C wrapper
            bus_num = int(port_name.split('-')[-1])
            self.get_logger().info(f"Opening I2C Bus: {bus_num}")
            self.ser = UbloxI2C(bus_num=bus_num, addr=0x42)
            self.ser.open()
        else:
            # Route to standard PySerial for UART ports (like ttyTHS1)
            import serial
            self.get_logger().info(f"Opening Serial Port: {port_name}")
            
            # Note: serial.Serial opens the port automatically when instantiated
            # but we can set it up to mimic your existing logic
            self.ser = serial.Serial()
            self.ser.port = port_name
            self.ser.baudrate = baudrate
            self.ser.timeout = 1
            self.ser.open()

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

    def nmea_loop(self):
        while self.running and rclpy.ok():
            try:
                if self.ser.in_waiting == 0:
                    time.sleep(0.005)  # Avoid busy waiting
                    continue

                with self.serial_lock:
                    _, msg_parsed = self.reader.read()

                if msg_parsed is not None:
                    with self.fix_lock:
                        self.process_nmea(msg_parsed)

            except Exception as e:
                self.get_logger().warn(f'❌ Failed to read NMEA message: {e}', throttle_duration_sec=1)
                self.get_logger().info(f'Reopening serial port...', throttle_duration_sec=1)
                try:
                    self.ser.close()
                    self.ser.open()
                except Exception as new_e:
                    self.get_logger().warn(f'❌ Failed to re-open serial port: {new_e}', throttle_duration_sec=1)
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

    def ubx_loop(self):
        while self.running and rclpy.ok():
            try:
                if self.ser.in_waiting == 0:
                    time.sleep(0.005)  # Avoid busy waiting
                    continue

                with self.serial_lock, self.fix_lock:
                    self.read_ubx()

            except Exception as e:
                self.get_logger().warn(f'❌ Failed to read UBX message: {e}', throttle_duration_sec=1)
                self.get_logger().info(f'Reopening serial port...', throttle_duration_sec=1)
                try:
                    self.ser.close()
                    self.ser.open()
                except Exception as new_e:
                    self.get_logger().warn(f'❌ Failed to re-open serial port: {new_e}', throttle_duration_sec=1)
                time.sleep(0.01)
    
    def read_ubx(self):
        try:
            # UBXReader does all the heavy lifting of finding headers and validating checksums
            (raw_data, parsed_data) = self.ubx_reader.read()
            
            if parsed_data is not None:
                # We only care about NAV-PVT for this node
                if parsed_data.identity == 'NAV-PVT':
                    
                    # pyubx2 automatically scales lat/lon (degrees) and height (mm)
                    self.pose.latitude = float(parsed_data.lat)
                    self.pose.longitude = float(parsed_data.lon)
                    self.pose.altitude = float(parsed_data.hMSL) / 1000.0       # mm to m
                    self.pose_custom.heading = float(parsed_data.headMot) / 1e5 # deg * 1e-5 to deg
                    
                    num_sv = parsed_data.numSV
                    fix_id = parsed_data.fixType # 0=No fix, 2=2D, 3=3D, 4=GNSS+DR
                    
                    fix_options = {0:"No Fix", 2:"2D Fix", 3:"3D Fix", 4:"GNSS+Dead Reckoning"}
                    self.fix_type = fix_options.get(fix_id, "Unknown")

                    if fix_id in [2, 3, 4]:
                        self.pose.status.status = NavSatStatus.STATUS_FIX
                    else:
                        self.pose.status.status = NavSatStatus.STATUS_NO_FIX
                        self.fix_type = "No fix"
                        self.get_logger().warn(f'❌ UBX GNSS fix not available!', throttle_duration_sec=1)

                    ### LOG ###
                    msg_log = f'''
                        {'-'*30}
                        🛰️ UBX Data:
                        \tlat: {self.pose.latitude:8.7f}
                        \tlon: {self.pose.longitude:8.7f}
                        \talt: {self.pose.altitude:8.7f}
                        \theading: {self.pose_custom.heading:8.3f}
                        \tfix type: {self.fix_type}
                        \tsatellite number: {num_sv}
                        {'-'*30}
                    '''
                    self.get_logger().info(msg_log, throttle_duration_sec=1)
                    
                    # Copy data to custom message
                    self.pose_custom.header = self.pose.header
                    self.pose_custom.status = self.pose.status
                    self.pose_custom.latitude = self.pose.latitude
                    self.pose_custom.longitude = self.pose.longitude
                    self.pose_custom.altitude = self.pose.altitude

        except Exception as e:
            # Catch stream errors, checksum failures, or I2C bus collisions
            self.get_logger().debug(f"UBX Read Error: {e}")

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
