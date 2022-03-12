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
AUTHOR(S):	Liam Whittle
CREATION:	08/03/2022
EDITED:		08/03/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - change all the template artefacts
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node

# example of how to import a custom message type
from core.msg import RoverPose

# an example of how to import a standard message type
from std_msgs.msg import String


class TemplateNode(Node):

    def __init__(self):
        super().__init__("TemplateNode")
        # way-point publisher publishes a bunch of waypoints at once (hence using the 2D map datatype
        self.subscriber = self.create_subscription(RoverPose, "/template/subscriber", self.subscriber_callback, 10)
        # current state of internal message
        self.msg = RoverPose()

        self.publisher = self.create_publisher(String, "/template/publisher", 10)
        timer_period = 0.1  # run the timer 10 times per second
        self.create_timer(timer_period, self.timer_callback)

    def subscriber_callback(self, msg):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        self.msg = msg

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
