#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from std_srvs.srv import Empty  # or your service type
from arm_interfaces.srv import TypeSequence

"""
Run with: ros2 run nova_utils service_listener.py
"""

class ListenNode(Node):
    def __init__(self):
        super().__init__('service_listener')
        self.srv = self.create_service(TypeSequence, '/type_sequence/start', self.callback)

    def callback(self, request, response):
        self.get_logger().info(f'Service called: {request.sequence}')
        return response

def main(args=None):
    rclpy.init(args=args)
    node = ListenNode()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == "__main__":
    main()