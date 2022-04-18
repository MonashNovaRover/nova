#!/usr/bin/env python

'''
NOVA ROVER TEAM
This script is a ros service that handles communicating commands to the LED Lights.
Authors: Max Tory and Marcel Masque
Last Modified: 14/04/2022 By Max Tory
'''
import rclpy
from rclpy.node import Node
from core.srv import LED
from core.msg import InputGamepad
from coms_utils.can_interface import CANTransmitter
import time

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
    def __init__(self):
        self.transmitter = CANTransmitter(
                channel='can0', # Can channel to transmit on
                arbitration_id=0x1f,  # ID - lower if too high idk)
                transmit_timeout=0.1, # Timeout in seconds - keep low so we can rave
                transmit_labels=['r', 'g', 'b'], 
                transmit_fmt='<BBB' # 3 unsigned chars in little endian format
                )

    def set_LED(self, r, g, b):
        print(f"Setting LED to [R: {r}, G: {g}, B: {b}]")
        """TODO: actually interface with CAN
        """
        
        packed_data = self.transmitter.pack([r, g, b])
        ret = self.transmitter.transmit(packed_data)

        return ret  # for informing of errors
     
class LEDTransmitterServiceNode(Node, CanLEDCommunicator):
    """This class' job is to receive requests for setting the LED colour, and 
    passing the request on via CAN. 
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

    Note: this is a temporary approach. Really, we should be putting heartbeat as a mode
    in a topic with the other mode types, and then this should be a subscriber to that topic.
    Then, a callback function will get executed periodically as the topic is published to. 
    """
    OFF = 0
    MANUAL = 1
    AUTONOMOUS = 2
    AUTO_GOAL_ACHIEVED = 3
    DISCONNECTED = 4
    def __init__(self):
        super().__init__('LED_status_update_node')
        # these should not be here! they should be in a file/params? and then 
        # can be pulled here
        self.gamepad_input_subscriber = self.create_subscription("/control/input_gamepad", InputGamepad, self.gamepad_callback, 1)
        self.qos_timer = self.create_timer(1.0, self.check_connection)

        self.mode_colours = {
            LEDStatusUpdateNode.OFF: [0, 0, 0],
            LEDStatusUpdateNode.MANUAL: [0, 0, 255],
            LEDStatusUpdateNode.AUTONOMOUS: [255, 127, 0],
            LEDStatusUpdateNode.AUTO_GOAL_ACHIEVED: [0, 255, 0]
            LEDStatusUpdateNode.DISCONNECTED: [255, 0, 0]
        }

        self.most_recent_update = time.perf_counter()

        self.mode = LEDStatusUpdateNode.MANUAL
     
    def check_connection():
        """
        simple method called every second to check that the node has still
        been receiving Gamepad messages
        """
        dt = time.perf_counter() - self.most_recent_update
        if dt > 1:
            self.mode = LEDStatusUpdateNode.DISCONNECTED

    def gamepad_callback(self, message):
        """
        callback that checks for the button B or A pressed on the controller and updates the state accordingly
        :param message: InputGamepad.msg type
        """
        B = message.btn_b_state
        A = message.btn_a_state

        if (B == 1 or B == 2):
            new_mode = LEDStatusUpdateNode.MANUAL
        elif (A == 1 or A == 2):
            new_mode = LEDStatusUpdateNode.AUTONOMOUS
        if (new_mode != self.mode):
            self.mode = new_mode
            self.update_LED_status()
        self.most_recent_update = time.perf_counter()

    def get_mode_RGB(self):
        """Gets RGB for the mode
        """
        return self.mode_colours[self.mode]

    def update_LED_status(self):
        """
        Asks the LED to update its colours when status changes occur
        """
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
