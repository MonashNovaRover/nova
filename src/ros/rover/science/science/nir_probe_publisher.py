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

import rclpy, jcan
from rclpy.node import Node
import random

# import custom messages
from core.msg import NIRProbeData
from core.srv import SetNIRProbeLED



class NIRProbePublisher(Node):
    CAN_BUS = "can1"
    # Card IDs
    NIR_PROBE_ID = 0x070
    CARD_ID_RECEIVE = 0x4F1
    # Command data
    NIR_PROBE_LED_ON = 0x01
    NIR_PROBE_LED_OFF = 0x02
    NIR_PROBE_READ = 0x03

    def __init__(self):
        super().__init__('nir_probe_publisher')

        self.declare_parameter("can_bus", self.CAN_BUS)

        # TODO: remove state from publisher, and use data from CAN
        self.led = (0).to_bytes(1)
        self.value = 0
        random.seed(None)

        self.publisher_ = self.create_publisher(NIRProbeData, '/science/nir_probe_data', 10)

        # TODO: replace callback with function that interfaces with CAN
        self.timer = self.create_timer(0.1, self.send_read_command_callback)

        self.led_service = self.create_service(SetNIRProbeLED, '/science/set_nir_probe_led', self.led_service_callback)

        self.bus = jcan.Bus()
        self.bus.set_id_filter_mask(self.CARD_ID_RECEIVE, 0xFFF)

        self.bus.add_callback(self.CARD_ID_RECEIVE, self.read_data_callback)

        self.bus.open(self.param_can)


    def send_read_command_callback(self):
        frame = jcan.Frame(self.NIR_PROBE_ID, [self.NIR_PROBE_READ])
        
        try:
            self.bus.send(frame)
        except Exception as e:
            print(e)
            self.get_logger().error("Failed to send read command over CAN")


    def read_data_callback(self, frame: jcan.Frame):
        if frame.id != self.CARD_ID_RECEIVE:
            self.get_logger().info(f"Received unknown frame {frame}")
            return

        self.value = int.from_bytes(frame.data)

        msg = NIRProbeData()
        msg.data = self.value

        msg.led = self.led

        self.publisher_.publish(msg)

            

    def led_service_callback(self, request, response):
        self.led = 1 if request.led > 0 else 0

        frame
        if self.led == 1:
            frame = jcan.Frame(self.NIR_PROBE_ID, [self.NIR_PROBE_LED_ON])
        else:
            frame = jcan.Frame(self.NIR_PROBE_ID, [self.NIR_PROBE_LED_OFF])

        try:
            self.bus.send(frame)
            response.success = True
        except Exception as e:
            print(e)
            self.get_logger().error("Failed to send led command over CAN")
            response.success = False

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
