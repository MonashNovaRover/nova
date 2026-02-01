#!/usr/bin/python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This node ensures that a predefined list of 
required ROS 2 topics is active before continuing 
execution. It is primarily used to synchronize 
startup logic and ensure all sensor streams are
publishing before other nodes depend on them.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: topic_waiter
PARAMS:
- topics (list of str): List of topic names to wait for.
- interval (float): Time in seconds between checks for topic availability.
- timeout (float): Maximum time in seconds to wait for topics before exiting with failure.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUTHOR(S):	Chetan Karthik Edupalli, Terry Tian
CREATION:	26/09/2025
MODIFIED:   01/02/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node


class TopicWaiter(Node):
    def __init__(self):
        super().__init__('topic_waiter')
        self.declare_parameter('topics', ['/rosout']) # 'list of topics to wait for'
        self.declare_parameter('interval', 1.0) # 'interval in seconds to check for topics'
        self.declare_parameter('timeout', float('inf')) # 'timeout in seconds to wait for topics'
        self.topics = self.get_parameter('topics').get_parameter_value().string_array_value
        self.timeout = self.get_parameter('timeout').get_parameter_value().double_value
        self.interval = self.get_parameter('interval').get_parameter_value().double_value
        self.start_time = self.get_clock().now().nanoseconds / 1e9

        self.get_logger().info(f'Waiting for topics: {self.topics}')
        self.timer = self.create_timer(self.interval, self.check_topics)

    def check_topics(self):
        all_active = True
        for t in self.topics:
            if not self.count_publishers(t):
                all_active = False
                self.get_logger().info(f'Waiting for topic: {t}')
                break
        
        if all_active:
            self.get_logger().info('All required topics are active, exiting.')
            rclpy.shutdown()
        elif (self.get_clock().now().nanoseconds / 1e9 - self.start_time) > self.timeout:
            self.get_logger().error('Timed out waiting for topics.')
            rclpy.shutdown()
        

def main(args=None):
    rclpy.init(args=args)
    topic_waiter = TopicWaiter()
    rclpy.spin(topic_waiter)


if __name__ == '__main__':
    main()
