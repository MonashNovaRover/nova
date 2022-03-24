#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: A node to convert raw wheel data into a 
format that the tracking camera ros node can accept
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: path_planner_node
TOPICS:
  - subscriber: /template/subscriber [RoverPose]
  - publisher: /template/publisher [String]
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Liam Whittle
CREATION:	13/03/2022
EDITED:		13/03/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - do some better outlier analysis for wheels on real data 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node

# example of how to import a custom message type
from core.msg import WheelData

# an example of how to import a standard message type
from nav_msgs.msg import Odometry 


class WheelOdomPublisher(Node):
    def __init__(self):
        super().__init__("tracking_wheels_pub")
        # subscriber subscribes to raw wheel encoder data
        self.subscriber = self.create_subscription(WheelData, "/electronics/wheel_data", self.subscriber_callback, 10)

        # current state of internal message
        self.msg = WheelData()
        
        # timer to publish wheel data
        self.publisher = self.create_publisher(Odometry, "/t265/wheels/in", 10)

        self.timer_period = 0.05  # run the timer 10 times per second
        self.create_timer(self.timer_period, self.timer_callback)

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
        odom_msg = Odometry()
        
        # calculate the average of all the instantaneous velocities
        average_vel = self.msg.velocities.sum() / 6.0

        odom_msg.twist.twist.linear.x = average_vel
        self.publisher.publish(odom_msg)


def main():
    rclpy.init()
    wheel_pub = WheelOdomPublisher()
    rclpy.spin(wheel_pub)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
