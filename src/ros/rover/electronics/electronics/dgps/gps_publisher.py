#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Read rover gps (skytraq) data from USB. Extract
relevant data and publish to network
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: SkytraqNode
TOPICS:
  - publisher: /electronics/gps_data [RoverPoseGPS]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	shelby n, will middlewick
CREATION:	25/02/2023
EDITED:		16/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - convert log to debug
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
GPS MODULE TYPE
supported types: ublox, skytraq
"""
GPS_MODULE = "ublox"
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from serial import Serial
from pynmeagps import NMEAReader, NMEAMessage
import re
import rclpy
from rclpy.node import Node

from nova_interfaces.msg import RoverPoseGPS
import logging

class SkytraqNode (Node):
    def __init__ (self, com_no, baud):
        super().__init__('gps_data')
        self.param_max_calibration_error = self.declare_parameter("max_calibration_error_degrees", 5e-5).value

        self.pose : RoverPoseGPS = RoverPoseGPS()
        self.pose.header.frame_id = "gps_link"

        self.fix_type : str = None

        self.ser : Serial = Serial()
        self.config_port(com_no, baud)
        self.reader = NMEAReader(
            self.ser,
            validate=0x03,   # validate both checksum and message id
            nmeaonly=True    # Raise an error on receiving a badly formatted message
        )

        self.publisher = self.create_publisher(RoverPoseGPS, '/electronics/gps_data', 10)
        self.timer = self.create_timer(0, self.publisher_callback)

    def parse_msg(self):
        
        self.pose.header.stamp = self.get_clock().now().to_msg()
        raw_msg : NMEAMessage
        try:
            raw_msg, parsed_msg = self.reader.read()
        except Exception as e:
            self.get_logger().warn(f"Failed to read NMEA sentence: {e}")
            return

        self.get_logger().debug(f"raw message: {raw_msg}")
        self.get_logger().debug(f"parsed message: {parsed_msg}")

        if parsed_msg is None:
            return

        if GPS_MODULE == "ublox":
            # wills code for the ublox module (16/04/25)
            ublox_msg_raw = str(parsed_msg)
            if 'lat=' in ublox_msg_raw:
                print("\nUblox GPS Module Data:")
                print("\traw=", ublox_msg_raw)
                match_lat = re.search(r'lat=([-\d.]+)', ublox_msg_raw)
                match_lon = re.search(r'lon=([-\d.]+)', ublox_msg_raw)
                latitude = 0
                longtitude = 0

                if match_lat:
                    latitude = float(match_lat.group(1))
                    print("\tlatitude=", latitude)
                
                if match_lon:
                    longtitude = float(match_lon.group(1))
                    print("\tlongitude=", longtitude)  

                if match_lat or match_lon:
                    self.pose.valid = True
                    self.pose.latitude, self.pose.longitude = latitude, longtitude
                    self.pose.heading_valid = False # Not RTK mode. We don't have valid heading
                    self.publisher.publish(self.pose)
                else: 
                    print("\tGPS data not available...")
                    self.pose.valid = False
            

        elif GPS_MODULE == "skytraq":
            try:
                if parsed_msg.talker == "P" and parsed_msg.msgID == "STI" and parsed_msg.msgId == "036":
                    # We are dealing with a PSTI036 message, which contains orientation information
                    if parsed_msg.mode == "R":
                        # RTK (Real-Time Kinematic) mode. We have valid heading
                        self.pose.heading_valid = True
                        self.pose.pitch, self.pose.roll, self.pose.yaw = parsed_msg.pitch, parsed_msg.roll, parsed_msg.heading
                    else:
                        # Not RTK mode. We don't have valid heading
                        self.pose.heading_valid = False

                elif parsed_msg.talker == "GN" and parsed_msg.msgID == "RMC":
                    if parsed_msg.status == 'A':
                        # Valid
                        self.pose.valid = True
                        self.pose.latitude, self.pose.longitude = parsed_msg.lat, parsed_msg.lon
                        self.publisher.publish(self.pose)
                    else:
                        self.pose.valid = False

                elif parsed_msg.talker == "GP" and parsed_msg.msgID == "GGA":
                    self.fix_type = parsed_msg.quality   # 1 = No fix, 2 = 2D fix, 3 = 3D fix
            except Exception as e:
                self.get_logger().warn(f"Bad message {parsed_msg}")

    def config_port(self, port_name, baud):
        self.ser.baudrate = baud
        if port_name == "":
            port_name = "/dev/ttyACM0"
        self.ser.port = port_name
        self.ser.open()

    def print_msg(self):
        roverMsgStr = f"""
        valid: {self.pose.valid}
        fix type: {"None" if self.fix_type == 1 else "2D" if self.fix_type == 2 else "3D" if self.fix_type == 3 else self.fix_type}
        lat: {self.pose.latitude:8.3f}
        lon: {self.pose.longitude:8.3f}
        pitch: {self.pose.pitch:8.2f}
        roll: {self.pose.roll:8.2f}
        yaw: {self.pose.yaw:8.2f}
        """

        if self.pose.valid:
            self.get_logger().debug(roverMsgStr,throttle_duration_sec=2)
        else:
            self.get_logger().warn(f'{roverMsgStr}',throttle_duration_sec=2)

    def publisher_callback(self):
        self.parse_msg()
        self.print_msg()

        
def main (args = None):
    baud = 115200
    rclpy.init(args = args)
    gps = SkytraqNode("", baud)
    rclpy.spin(gps)
    
    gps.destroy_node()
    rclpy.shutdown()
    
if __name__ == "__main__":
    main()
