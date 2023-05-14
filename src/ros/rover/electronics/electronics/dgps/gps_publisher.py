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

class SkytraqNode (Node):
    def __init__ (self, com_no, baud):
        super().__init__('gps_data')

        self.pose : RoverPoseGPS = RoverPoseGPS()
        self.pose.header.frame_id = "gps_link"

        self.fix_type : str = None

        self.ser : Serial = Serial()
        self.config_port(com_no, baud)
        self.reader = NMEAReader(
            self.ser,
            validate=0x03   # validate both checksum and message id
        )

        self.publisher = self.create_publisher(RoverPoseGPS, '/electronics/gps_data', 10)
        self.timer = self.create_timer(0, self.publisher_callback)

    def parse_msg(self):
        
        self.pose.header.stamp = self.get_clock().now().to_msg()
        raw_msg, parsed_msg : NMEAMessage = self.reader.read()
        self.get_logger().debug(f"raw message: {raw_msg}")

        if parsed_msg is None:
            self.get_logger().warn(f"Received message that doesn't fit specifications: {raw_msg}")
            return

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

        elif parsed_msg.talker == "GN" and parsed_msg.msgID == "GSA":
            self.fix_type = parsed_msg.navMode   # 1 = No fix, 2 = 2D fix, 3 = 3D fix

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
        self.parse_msg()
        self.publisher.publish(self.pose)
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
