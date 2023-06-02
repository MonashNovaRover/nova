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
AUTHOR(S):	shelby n
CREATION:	25/02/2023
EDITED:		25/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - convert log to debug
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from serial import Serial
from pynmeagps import NMEAReader, NMEAMessage

import rclpy
from rclpy.node import Node

from core.msg import RoverPoseGPS
from core.srv import GpsOffset
import logging

class SkytraqNode (Node):
    def __init__ (self, com_no, baud):
        super().__init__('gps_data')
        self.get_logger().set_level(logging.INFO)

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

        self.lat_offset = 0
        self.lon_offset = 0

        self.offset_service = self.create_service(GpsOffset, '/electronics/gps_offset', self.offset_callback)

        self.publisher = self.create_publisher(RoverPoseGPS, '/electronics/gps_data', 10)
        self.timer = self.create_timer(1/10, self.publisher_callback)

    def parse_msgs(self):
        
        self.pose.header.stamp = self.get_clock().now().to_msg()
        raw_msg : NMEAMessage
        try:
            data = [(raw_msg, parsed_msg) for raw_msg, parsed_msg in self.reader]
        except Exception as e:
            self.get_logger().warn(f"Failed to read NMEA sentence: {e}")
            return

        for raw_msg, parsed_msg in data:
            self.get_logger().debug(f"raw message: {raw_msg}")

            if parsed_msg is None:
                return

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
                    else:
                        self.pose.valid = False

                elif parsed_msg.talker == "GP" and parsed_msg.msgID == "GGA":
                    self.fix_type = parsed_msg.quality   # 1 = No fix, 2 = 2D fix, 3 = 3D fix
            except Exception as e:
                self.get_logger().warn(f"Bad message {parsed_msg}")

    def config_port(self, port_name, baud):
        self.ser.baudrate = baud
        if port_name == "":
            port_name = "/dev/ttyUSB0"
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
        self.parse_msgs()
        self.pose.latitude += self.lat_offset
        self.pose.longitude += self.lon_offset
        self.publisher.publish(self.pose)
        self.print_msg()

    def offset_callback(self, request: GpsOffset.Request, response: GpsOffset.Response):
        """
        Accepts an accurate GPS coordinate for the current position of the rover.
        Stores an offset for the latitude and longitude we are currently receiving, and will
        correct by that offset on all future measurements, unless calibrated again for another position
        """
        if self.pose.valid:
            true_lat = request.lat
            true_lon = request.lon
            lat_offset = true_lat - self.pose.latitude
            lon_offset = true_lon - self.pose.longitude

            # If error is too great assume we have given a bad coordinate, don't calibrate
            if lat_offset > self.param_max_calibration_error or lon_offset > self.param_max_calibration_error:
                response.success = False
                response.message = "Error too great"
            else:
                self.lat_offset = lat_offset
                self.lon_offset = lon_offset
                response.success = True
                response.message = "Good"
        else:
            response.success = False
            response.message = "No rover fix"
        return response

        
def main (args = None):
    baud = 115200
    rclpy.init(args = args)
    gps = SkytraqNode("", baud)
    rclpy.spin(gps)
    
    gps.destroy_node()
    rclpy.shutdown()
    
if __name__ == "__main__":
    main()
