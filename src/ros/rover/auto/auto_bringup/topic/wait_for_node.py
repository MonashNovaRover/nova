#!/usr/bin/python3
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
    Bartlinski
CREATION:	26/09/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data
from rclpy.parameter import Parameter
import sys
import time


REQUIRED_NODES = [
    '/parameter_blackboard',
]

CHECK_INTERVAL = 1.0  # seconds
TIMEOUT = 5  # seconds to give up


class NodeWaiter(Node):
    def __init__(self):
        super().__init__('node_waiter')
        self.start_time = time.time()

    def all_nodes_active(self):
        available_nodes = [t[0] for t in self.get_node_names()]
        return all(node in available_nodes for node in REQUIRED_NODES)

    def spin_until_ready(self):
        self.get_logger().info(f"Waiting for required nodes: {REQUIRED_NODES}")
        while rclpy.ok():
            rclpy.spin_once(self, timeout_sec=CHECK_INTERVAL)
            if self.all_nodes_active():
                self.get_logger().info("All required nodes are active.")
                return True
            if (time.time() - self.start_time) > TIMEOUT:
                self.get_logger().error("Timed out waiting for nodes.")
                return False
            time.sleep(CHECK_INTERVAL)


def main(args=None):
    rclpy.init(args=args)
    node = NodeWaiter()

    success = node.spin_until_ready()
    node.destroy_node()
    rclpy.shutdown()

    if not success:
        sys.exit(1)


if __name__ == '__main__':
    main()
