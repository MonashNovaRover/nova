#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This node ensures that a predefined list of 
required ROS 2 nodes are active before continuing 
execution. It is primarily used to synchronize 
startup logic and ensure all sensor streams are
publishing before other nodes depend on them.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: node_waiter
TOPICS: None
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUTHOR(S):	Chetan Karthik Edupalli, Victor 
    Bartlinski, Terry Tian
CREATION:	26/09/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from rclpy.parameter import Parameter
import sys
import time

class NodeWaiter(Node):
    def __init__(self):
        super().__init__('node_waiter')
        self.declare_parameter('interval', 1.0) # 'interval in seconds to check for topics'
        self.declare_parameter('nodes', ['parameter_blackboard']) # 'list of nodes to wait for'
        self.declare_parameter('timeout', float('inf')) # 'timeout in seconds to wait for topics'
        self.interval = self.get_parameter('interval').get_parameter_value().double_value
        self.nodes = self.get_parameter('nodes').get_parameter_value().string_array_value
        self.timeout = self.get_parameter('timeout').get_parameter_value().double_value
        self.start_time = self.get_clock().now().nanoseconds / 1e9

        self.get_logger().info(f'Waiting for nodes: {self.nodes}')
        self.timer = self.create_timer(self.interval, self.check_nodes)

    def check_nodes(self):
        active_nodes = self.get_node_names()
        self.get_logger().debug(f'Active nodes: {active_nodes}')
        
        if all(node in active_nodes for node in self.nodes):
            self.get_logger().info('All required nodes are active, exiting.')
            rclpy.shutdown()
        elif (self.get_clock().now().nanoseconds / 1e9 - self.start_time) > self.timeout:
            self.get_logger().error('Timed out waiting for nodes.')
            rclpy.shutdown()


def main(args=None):
    rclpy.init(args=args)
    node_waiter = NodeWaiter()
    rclpy.spin(node_waiter)

if __name__ == '__main__':
    main()