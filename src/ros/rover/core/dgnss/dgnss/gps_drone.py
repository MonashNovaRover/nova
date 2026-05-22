#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: connect to MAVLink UDP port opened by MAVProxy.
Use this connection to read location (lat, long, alt) from
the drone and publish this to ros2 topic /gps_drone/fix_custom
so that the cartographer can read this and add the
drone to the GUI
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gps_drone
TOPICS:
  - publisher: /gps_drone/fix_custom [GPSData]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        drone
AUTHOR(S):      Henry Law, Victor Bartlinski
CREATION:       15/4/2026
EDITED:         15/4/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
from pymavlink import mavutil
from nova_interfaces.msg import GPSData

class DroneGPS(Node):
    DEVICE_PARAM = "device"
    BAUD_PARAM = "baudrate"
    TIMEOUT_PARAM = "timeout"


    def __init__(self):
        super().__init__("gps_drone")
        
        # define variables
        self.connected = False
        self.gps_connected = False

        # declare parameters
        self.declare_parameter(self.DEVICE_PARAM, "udp:127.0.0.1:14551")
        self.declare_parameter(self.BAUD_PARAM, 460800)
        self.declare_parameter(self.TIMEOUT_PARAM, 0.2)
        # Create connection object
        self.connection = mavutil.mavlink_connection(
            self.get_parameter(self.DEVICE_PARAM).value, 
            baud=self.get_parameter(self.BAUD_PARAM).value)
        
        # add publishers
        self.gps_publisher = self.create_publisher(GPSData, '/gps_drone/fix_custom', 10)
        
        # create a timer
        self.PERIOD = 0.5
        self.update_timer = self.create_timer(self.PERIOD, lambda: self.update(self.PERIOD))


        self.get_logger().info(f"{self.get_name()} started.")

    def connect(self):
        msg = self.connection.wait_heartbeat(timeout=self.get_parameter(self.TIMEOUT_PARAM).value)
        if (msg is None):
            return False
        self.connected = True
        self.get_logger().info(f"{self.get_name()} connected!")
        return True

    def get_gps(self, msg):
        if (not self.gps_connected):
            self.get_logger().info(f"{self.get_name()} Fetching GPS...")
            pos_msg = self.connection.recv_match(type="GLOBAL_POSITION_INT")
            if (pos_msg is not None):
                self.gps_connected = True
                self.get_logger().info(f"{self.get_name()} GPS connected")
        else:
            pos_msg = self.connection.recv_match(type="GLOBAL_POSITION_INT")
        
        if (pos_msg is not None):
            # Position
            msg.latitude = pos_msg.lat / 1e7
            msg.longitude = pos_msg.lon / 1e7
            msg.altitude = pos_msg.relative_alt / 1000.0

            if pos_msg.hdg == 65535:
                msg.heading = -1.0
            else:
                msg.heading = pos_msg.hdg / 100.0


    def update(self, delta_time):
        if (not self.connected):
            if (self.connect() is False):
                return

        # Construct the message you want to send
        msg = GPSData()
        
        self.get_gps(msg)
        
        if (msg is not None):
            # Publish the message
            self.gps_publisher.publish(msg)

    def destroy_node(self):
        try:
            self.connection.close()
        except Exception:
            pass
        super().destroy_node()

def main():
    rclpy.init()
    node = DroneGPS()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
