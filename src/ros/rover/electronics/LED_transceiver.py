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
class CanLEDCommunicator():
     """Handles communication with LED
     """
     def set_LED(self, r, g, b):
          print(f"Setting LED to [R: {r}, G: {g}, B: {b}]")
          """TODO: actually interface with CAN
          """
     
class LEDTransceiverServiceNode(Node,CanLEDCommunicator):
     """This class' job is to receive requests for setting the LED colour, and 
     passing the request on via CAN. 
     TODO: Pass the request on via CAN (currently prints out requests)
     """
     def __init__(self):
          super().__init__('LED_transceiver_node')
          # The service that listens for LED setting requests
          self.service = self.create_service(LED, 'LED', self.callback)
     
     def callback(self, request, response):
          self.get_logger().info(f"Service received request with data [R: {request.r}, G: {request.g}, B: {request.b}]")
          self.set_LED(request.r, request.g, request.b)
          response.sent_status = True

          return response

class LEDStatusUpdateNode(Node,CanLEDCommunicator):
     """class: its job is to check the status of the rover and update the LED via can on changes
     TODO: update the LED via can on changes

     Note: this is a temporary approach. Really, we should be putting heartbeat as a mode
     in a topic with the other mode types, and then this should be a subscriber to that topic.
     Then, a callback function will get executed periodically as the topic is published to. 
     """ 
     def __init__(self):
          super().__init__('LED_status_update_node')
          # these should not be here! they should be in a file/params? and then 
          # can be pulled here
          self.modes = {
               0: "off",
               1: "xbox", 
               2: "phone", 
               3: "auto"
          }
          self.mode_colours = {
               "off":[0,0,0],
               "xbox":[0,255,0],
               "phone":[255,200,0],
               "auto":[0,0,255],
               "heartbeat":[255,0,0]
          }
          self.mode = 'off'
     
     def get_rover_heartbeat(self):
          heartbeat = True    # TODO actually get heartbeat
          return heartbeat

     def get_rover_mode(self):
          """ Gets the rover's current mode according to publisher/param and heartbeat
          TODO: get mode from publisher
          """
          heartbeat = self.get_rover_heartbeat()
          if heartbeat:  # hearbeat established
               mode = 1 # TODO: get the mode from param/publisher
               return self.modes[mode]
          else:     # no hearbeat, so we are in heartbeat mode
               return 'heartbeat'

     def get_mode_RGB(self):
          """Gets RGB for the mode
          """
          return self.mode_colours[self.mode]

     def update_LED_status(self):
          """Monitors for changes in the LED status, and asks the LED 
          to update its colours when status changes occur
          """
          # get the current rover mode
          updated_mode = self.get_rover_mode()

          # the mode changed, so update the RGB status 
          if updated_mode != self.mode:
               print("mode changed")
               self.mode = updated_mode
               colours = self.get_mode_RGB()
               self.set_LED(*colours)
          

def main (args = None):
     rclpy.init(args=args)
     service = LEDTransceiverServiceNode()
     LEDupdater = LEDStatusUpdateNode()
     while rclpy.ok():
          LEDupdater.update_LED_status()
          rclpy.spin_once(service)
     service.destroy_node()
     rclpy.shutdown()

if __name__ == '__main__':
     main()
