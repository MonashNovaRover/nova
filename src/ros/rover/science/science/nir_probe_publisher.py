#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 receiver code for the
nir probe publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: nir_probe_publisher
TOPICS:
  - /science/nir_probe_data [NIRProbeData]
    [Published]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     science
AUTHOR(S):   Bailey Chessum
CREATION:    4/02/2024
EDITED:      9/02/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
  - Implement CAN communication with the NIR
    probe to produce real data
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
import random

# import custom messages
from core.msg import NIRProbeData
from core.srv import SetNIRProbeLED

class NIRProbePublisher(Node):
    def __init__(self):
        super().__init__('nir_probe_publisher')

        # TODO: remove state from publisher, and use data from CAN
        self.led = (0).to_bytes(1)
        random.seed(None)

        self.publisher_ = self.create_publisher(NIRProbeData, '/science/nir_probe_data', 10)

        # TODO: replace callback with function that interfaces with CAN
        self.timer = self.create_timer(0.1, self.test_timer_callback)

        self.led_service = self.create_service(SetNIRProbeLED, '/science/set_nir_probe_led', self.led_service_callback)

    def test_timer_callback(self):
        msg = NIRProbeData()

        msg.data = random.randrange(0, 1024)
        msg.led = self.led

        self.publisher_.publish(msg)
        self.get_logger().info('Publishing: "%s"' % msg.data)

    def led_service_callback(self, request, response):
        self.led = request.led

        response.success = True
        return response


# The main code that executes when starting
def main(args=None):
    # Create the publisher
    rclpy.init(args = args)
    publisher = NIRProbePublisher()
    rclpy.spin(publisher)

    # Clean up when complete
    publisher.destroy_node()
    rclpy.shutdown()


# Called when the script executes
if __name__=="__main__":
    main()
