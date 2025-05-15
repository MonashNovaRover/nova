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
from rclpy.qos import QoSProfile

from sensor_msgs.msg import NavSatFix

class GzGpsFixer(Node):

    def __init__(self):
        super().__init__('gz_gps_fixer')
        self.sub_gps = self.create_subscription(NavSatFix, '/gz/gps_rover/fix', self.sub_callback, 10)
        self.pub_gps = self.create_publisher(NavSatFix, '/gps_rover/fix', 10)

    def sub_callback(self, msg):
        msg.altitude = 0.0
        self.pub_gps.publish(msg)

def main(args=None):
    rclpy.init(args=args)
    node = GzGpsFixer()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()

