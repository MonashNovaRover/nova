#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gz_gps_fixer
TOPICS:
  - subscriber: /gz/gps_rover/fix   [NavSatFix]
  - publisher: /gps_rover/fix       [NavSatFix]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_utils
EDITED BY:	Victor Bartlinski
CREATION:	27/04/2025
EDITED:		30/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import NavSatFix
from nova_interfaces.msg import GPSData
import rclpy.qos as qos

class GzGpsFixer(Node):

    def __init__(self):
        super().__init__('gz_gps_fixer')
        self.sub_gps = self.create_subscription(NavSatFix, '/gps_rover/fix', self.sub_callback, 
            qos.QoSProfile(depth=1, reliability=qos.ReliabilityPolicy.BEST_EFFORT))
        self.pub_gps = self.create_publisher(GPSData, '/gps_rover/fix_custom', qos.QoSReliabilityPolicy.BEST_EFFORT)

    def sub_callback(self, msg):
        custom_msg = GPSData()
        custom_msg.header = msg.header
        custom_msg.status = msg.status
        custom_msg.latitude = msg.latitude
        custom_msg.longitude = msg.longitude
        custom_msg.altitude = msg.altitude
        self.pub_gps.publish(custom_msg)

def main(args=None):
    rclpy.init(args=args)
    node = GzGpsFixer()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()

