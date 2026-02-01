#!/usr/bin/python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This node ensures that a predefined list of 
required ROS 2 topics are active before continuing 
execution. It is primarily used to synchronize 
startup logic and ensure all sensor streams are
publishing before other nodes depend on them.

It listens for the specified topics and waits until
data is received on all of them, or until a timeout
is reached.

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
from rosidl_runtime_py.utilities import get_message


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
        self.topics_active = [False] * len(self.topics)
        self.subs = [None] * len(self.topics) # Store subscriptions to prevent garbage collection

        self.get_logger().info(f'Waiting for topics: {self.topics}')
        self.timer = self.create_timer(self.interval, self.check_topics)


    def check_topics(self):
        def get_topic_active_cb(i):
            def callback(_): # discard the msg
                self.topics_active[i] = True
                self.get_logger().info(f'Data received on topic: {self.topics[i]}')
                self.destroy_subscription(self.subs[i])
            return callback

        # Loop through topics and create subscriptions once they exist
        for i in range(len(self.topics)):
            if self.subs[i]:
                continue

            pubs = self.get_publishers_info_by_topic(self.topics[i])
            if pubs:
                # Create subscription to topic to wait for data
                self.topics_active[i] = False
                sub = self.create_subscription(
                    msg_type=get_message(pubs[0].topic_type),
                    topic=self.topics[i],
                    callback=get_topic_active_cb(i),
                    qos_profile=1,
                )
                self.subs[i] = sub
                self.get_logger().info(f'Subscribed to topic: {self.topics[i]}')
            else:
                self.get_logger().info(f'Waiting for topic to exist: {self.topics[i]}')
                break
        
        # All topics have received data
        if all(self.topics_active):
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
