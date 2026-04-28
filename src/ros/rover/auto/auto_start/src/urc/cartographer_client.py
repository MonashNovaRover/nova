#!/usr/bin/env python3
from rclpy.node import Node
from nova_interfaces.srv import CartographerCommand

class CartographerClient():
    def __init__(self, node:Node):
        self.node=node
        self.goals=[]
        self.goal_type=None
        self.search_radius=None
        self.started=False

        # Create service for Auto URC Cartographer GUI 
        self.cartographer_service = self.node.create_service(CartographerCommand, '/autonomous/cartographer_command', self.called)
        self.node.get_logger().info('Serving /autonomous/cartographer_command.')

    def called(self, request, response):
        '''Loads waypoints from GUI and converts them into PoseStamped messages.'''
        self.goals = request.goals
        self.goal_type = request.goal_type
        self.search_radius = request.search_radius
        while not self.response:
            pass
        return response

    def received_goals(self):
        return len(self.goals) > 0

    def respond(self, response:bool):
        self.response = response