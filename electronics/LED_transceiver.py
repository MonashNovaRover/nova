#!/usr/bin/env python

'''
NOVA ROVER TEAM
This script is a ros service that handles communicating commands to the LED Lights.
Authors: Max Tory and Marcel Masque
Last Modified: 27/09/2021 By Max Tory and Marcel Masque
'''
import rclpy
from rclpy.node import Node
from core.srv import LED

"""
Goal 1: get a request in the service, and execute
a callback function setting the colours.
     - for now is printed out to screen 
     - can bus not implemented

Goal 2: whenever the rover is running, check the mode and update
color on changes 
     - check_update + callback?
"""

class LEDTransceiverServiceNode(Node):

     def __init__(self):
          super().__init__('LED_transceiver_node')
          # The service that listens for LED setting requests
          self.service = self.create_service(LED, 'LED', self.callback)
     
     def callback(self, request, response):
          print("Callback")
          self.get_logger().info(f"Service received request with data [R: {request.r}, G: {request.g}, B: {request.b}]")
          response.sent_status = 1234
          return response

def main (args = None):
     rclpy.init(args=args)
     service = LEDTransceiverServiceNode()
     rclpy.spin(service)
     service.destroy_node()
     rclpy.shutdown()

if __name__ == '__main__':
     main()
