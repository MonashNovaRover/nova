#!/usr/bin/env python3
from rclpy.node import Node
from nova_interfaces.srv import CartographerCommand
import time

class CartographerClient():
    def __init__(self, node:Node):
        self.node=node
        self.goals=[]
        self.goal_type=None
        self.search_radius=None

        # Create service for Auto URC Cartographer GUI 
        self.cartographer_service = self.node.create_service(CartographerCommand, '/autonomous/cartographer_command', self.called)
        self.node.get_logger().info('Serving /autonomous/cartographer_command.')

    def called(self, request, response):
        '''Loads waypoints from GUI and converts them into PoseStamped messages.'''
        self.node.get_logger().info('Received /autonomous/cartographer_command request.')
        self.goals = request.goals
        self.goal_type = request.goal_type
        self.search_radius = request.search_radius
        response.success = True
        self.node.get_logger().info('Sent /autonomous/cartographer_command response.')
        return response

    def received_goals(self):
        return len(self.goals) > 0