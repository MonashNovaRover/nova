#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: connect to MAVLink UDP port opened by MAVProxy.
Use this connection to read location (lat, long, alt) from
the drone and publish this to ros2 topic /drone_gps/fix
so that the cartographer can read this and add the
drone to the GUI
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: drone_gps
TOPICS:
  - publisher: /drone_gps/fix [NavSatFix]
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
from sensor_msgs.msg import NavSatFix

class DroneGPS(Node):
    DEVICE_PARAM = "device"
    BAUD_PARAM = "baudrate"
    TIMEOUT_PARAM = "timeout"


    def __init__(self):
        super().__init__("DroneGPS")
        
        # define variables
        self.connected = False
        self.gps_connected = False

        # declare parameters
        self.declare_parameter(self.DEVICE_PARAM, "/dev/ttyACM1")
        self.declare_parameter(self.BAUD_PARAM, 460800)
        self.declare_parameter(self.TIMEOUT_PARAM, 0.2)
        # Create connection object
        self.connection = mavutil.mavlink_connection(
            self.get_parameter(self.DEVICE_PARAM).value, 
            baud=self.get_parameter(self.BAUD_PARAM).value)
        
        # add publishers
        self.gps_publisher = self.create_publisher(NavSatFix, '/drone_gps/fix', 10)
        
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
            location = self.connection.location()
            self.gps_connected = True
            self.get_logger().info(f"{self.get_name()} GPS connected")
        else:
            location = self.connection.location()
        
        msg.latitude, msg.longitude, msg.altitude = location.lat, location.lng, location.alt

    def update(self, delta_time):
        if (not self.connected):
            if (self.connect() is False):
                return

        # Construct the message you want to send
        msg = NavSatFix()
            
        self.get_gps(msg)
        
        # Publish the message
        self.gps_publisher.publish(msg)

def main():
    rclpy.init()
    node = DroneGPS()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()
