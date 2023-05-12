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
import serial

import rclpy
from rclpy.node import Node

from core.msg import RoverPoseGPS
from rclpy.logging import LoggingSeverity

class SkytraqNode (Node):
    def __init__ (self, com_no, baud):
        super().__init__('gps_data')

        self.pose : RoverPoseGPS = RoverPoseGPS()
        self.pose.header.frame_id = "gps_link"

        self.fix_type : str = None

        self.ser = serial.Serial()
        self.config_port(com_no, baud)

        self.publisher = self.create_publisher(RoverPoseGPS, '/electronics/gps_data', 10)
        self.timer = self.create_timer(1 / 30, self.publisher_callback)

    def parse_msg(self, pose : RoverPoseGPS):
        self.pose.header.stamp = self.get_clock().now().to_msg()
        raw_msg = self.get_msg()
        self.get_logger().debug(f"raw message: {raw_msg}")

        if raw_msg[0:2] == ["PSTI", '036']:
            if raw_msg[4] != '' and (raw_msg[4].isupper() or raw_msg[4].islower()) == False:
                pose.pitch, pose.roll, pose.yaw = float(raw_msg[5]), float(raw_msg[6]), float(raw_msg[4])
                pose.heading_valid = True
        
        elif raw_msg[0] == "GNRMC":
            pose.latitude, pose.longitude = float(raw_msg[3])/100, float(raw_msg[5])/100
            if raw_msg[4] == "S":
                pose.latitude = -1 * pose.latitude
            if raw_msg[6] == "W":
                pose.longitude = -1 * pose.longitude

            if raw_msg[2] == 'A':
                pose.valid = True
            else:
                pose.valid = False

            if raw_msg[12] != "":
                self.fix_type = raw_msg[12]
        
        elif raw_msg[0] == "GPGGA":
            self.get_logger().debug(f'fix type: {raw_msg[6]}')

    def get_msg(self):
        txt = str(self.ser.read_until(b"$"))
        txt = txt.rstrip("\\r\\n$'")
        txt = txt.lstrip("b'")
        return txt.split(",")
    
    def config_port(self, port_name, baud):
        self.ser.baudrate = baud
        if port_name == "":
            port_name = "/dev/ttyUSB0"
        self.ser.port = port_name
        self.ser.open()

    def print_msg(self, rover_msg):
        roverMsgStr = f"""
        valid: {rover_msg.valid}
        fix type: {self.fix_type}
        lat: {rover_msg.latitude:8.3f}
        lon: {rover_msg.longitude:8.3f}
        pitch: {rover_msg.pitch:8.2f}
        roll: {rover_msg.roll:8.2f}
        yaw: {rover_msg.yaw:8.2f}
        """

        if rover_msg.valid:
            self.get_logger().debug(roverMsgStr,throttle_duration_sec=2)
        else:
            self.get_logger().debug(f'[WARN] {roverMsgStr}',throttle_duration_sec=2)

    def publisher_callback(self):
        self.parse_msg(self.pose)
        self.publisher.publish(self.pose)
        self.print_msg(self.pose)

        
def main (args = None):
    baud = 115200
    rclpy.init(args = args)
    gps = SkytraqNode("", baud)
    rclpy.spin(gps)
    
    gps.destroy_node()
    rclpy.shutdown()
    
if __name__ == "__main__":
    main()
