#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: A template ROS node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: path_planner_node
TOPICS:
  - subscriber: /template/subscriber [RoverPose]
  - publisher: /template/publisher [String]
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Taaj Street
CREATION:	09/03/2023
EDITED:		09/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - change all the template artefacts
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
import jcan

# import custom messages
from core.msg import BLCMDReset, BLCMDStatus, BLCMDStatusArray

# import diagnostic_msgs
from diagnostic_msgs.msg import DiagnosticStatus, DiagnosticArray


class BLCMDStatusMonitor(Node):

    def __init__(self):
        super().__init__("BLCMDStatusMonitor")
        #subscriber to reset the blcmd
        self.subscriber = self.create_subscription(BLCMDReset, "/control/blcmd_reset", self.reset_blcmd, 10)
        #publisher to publish the status of the blcmd
        self.publisher = self.create_publisher(BLCMDStatusArray, "/control/blcmd_status", 10)
        self.declare_parameter("num_blcmds", 8)
        self.declare_parameter("can_bus", "can0")

        self.bus = jcan.Bus()
        self.bus.set_id_filter_mask(0x400, 0xF0F)

        for i in range(self.get_parameter(num_blcmds).value):
            self.bus.add_callback(0x400 | (i + 1) << 4, self.get_callback(i + 1))

        self.bus.open(self.get_parameter(can_bus).value)

    def reset_blcmd(self, msg):
        """
        Updates the classes internal msg state
        :param msg: core.msg.BLCMDReset message from the subscriber callback
        :return: None
        """
        self.bus.send(jcan.)



    def timer_callback(self):
        """
        Called every timer_period. Publishes to self.publisher
        :return:
        """
        string_msg = String()
        string_msg.data = "Rover's x coordinate: " + str(self.msg.x)
        self.publisher.publish(string_msg)


def main():
    rclpy.init()
    template_node = TemplateNode()
    rclpy.spin(template_node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
