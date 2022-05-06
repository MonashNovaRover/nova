#!/usr/bin/env python

'''
NOVA ROVER TEAM
This script is a ros service that handles communicating commands to the LED Lights.
Authors: Max Tory and Marcel Masque
Last Modified: 14/04/2022 By Max Tory

TODO: Add custom routines that do different light patterns and sequences - maybe we
want to flash if disconnected etc. Gives us more options since we can't combine
colours
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

"""


class CanLEDCommunicator:
    """Handles communication with LED
    """
    RED = 0x91
    GREEN = 0x92
    BLUE = 0x93

    def __init__(self):
        self.transmitter = CANTransmitter(
            channel='can0',  # Can channel to transmit on
            arbitration_id=0x91,  # ID for red can trasmitter
            transmit_timeout=0.1,  # Timeout in seconds between messages
            transmit_labels=['intensity'],
            transmit_fmt='B'  # unsigned char for intensity
        )

    def set_LED(self, colour, intensity):
        """
        Sends data to CAN bus to actually activate the LED array
        """
        print(f"Setting LED to {hex(colour)} with intensity of {intensity}")

        transmitter = self.transmitter
        transmitter.arbitration_id = colour  # send to the desired colour LED
        packed_data = transmitter.pack([intensity])
        ret = transmitter.transmit(packed_data)

        return ret  # for informing of errors


class LEDUpdateNode(Node):
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
        self.can_communicator = CanLEDCommunicator()
        # these should not be here! they should be in a file/params? and then 
        # can be pulled here
        self.gamepad_input_subscriber = self.create_subscription(InputGamepad, "/control/input_gamepad", self.gamepad_callback, 10)
        self.service = self.create_service(LED, 'autonomous/LED', self.service_callback)

        self.qos_timer = self.create_timer(1.0, self.check_connection)
        self.display_timer = self.create_timer(1.0, self.display)

        # Colours and their brightness for each mode (mostly half brightness to save power)
        self.mode_colours = {
            LEDUpdateNode.MANUAL: (CanLEDCommunicator.BLUE, 128),
            LEDUpdateNode.AUTONOMOUS: (CanLEDCommunicator.RED, 128),
            LEDUpdateNode.AUTO_GOAL_ACHIEVED: (CanLEDCommunicator.GREEN, 128),
            LEDUpdateNode.DISCONNECTED: (CanLEDCommunicator.RED, 255)
        }

        # Control how the above colour is displayed
        self.mode_functions = {
            LEDUpdateNode.MANUAL: self.display_continuous,
            LEDUpdateNode.AUTONOMOUS: self.display_continuous,
            LEDUpdateNode.AUTO_GOAL_ACHIEVED: self.display_continuous,
            LEDUpdateNode.DISCONNECTED: self.display_flash,
        }

        self.flash_duration = 1.0  # 1 second per flash

        self.most_recent_update = time.perf_counter()

        self.mode = LEDUpdateNode.MANUAL

    def check_connection(self):
        """
        simple method called every second to check that the node has still
        been receiving Gamepad messages
        """
        dt = time.perf_counter() - self.most_recent_update
        if dt > 1:
            self.mode = LEDUpdateNode.DISCONNECTED
        elif self.mode == LEDUpdateNode.DISCONNECTED:
            self.mode = LEDUpdateNode.MANUAL

    def gamepad_callback(self, message):
        """
        callback that checks for the button B or A pressed on the controller and updates the state accordingly
        :param message: InputGamepad.msg type
        """
        B = message.btn_b_state
        A = message.btn_a_state

        if (B == 1 or B == 2):  # B is pressed or held
            self.mode = LEDUpdateNode.MANUAL
        elif (A == 1 or A == 2):  # A is pressed or held
            self.mode = LEDUpdateNode.AUTONOMOUS

        if message.connected:
            self.most_recent_update = time.perf_counter()

    def service_callback(self, request, response):
        """self.get_logger().info(f"Service received request with data [R: {request.r}, G: {request.g}, B: {request.b}]")
        colour = 0x91 if request.r \
            else 0x92 if request.g \
            else 0x93 if request.b \
            else 0    # invalid request

        if colour == 0:
            response.sent_status = False
        else:
            intensity = request.intensity

            response.sent_status = self.can_communicator.set_LED(colour, intensity)
        """
        self.mode = LEDUpdateNode.AUTO_GOAL_ACHIEVED
        response.sent_status = True
        return response


    def display(self):
        """
        Display a colour based on the mode of the rover either continuous or flashing
        """
        self.mode_functions[self.mode]()  # Controls how the colour is displayed

    def display_continuous(self):
        """
        Displays a colour based on the current mode of the Rover
        """
        # get colour ad brightness
        colour_info = self.mode_colours[self.mode]
        self.can_communicator.set_LED(*colour_info)

    def display_flash(self):
        """
        sets light to off for half the duration, and on for the second half
        """
        off = (CanLEDCommunicator.RED, 0)  # 0 brightness message
        self.can_communicator.set_LED(*off)  # Turn off

        time.sleep(self.flash_duration / 2)
        self.display_continuous()  # Turn back on
        time.sleep(self.flash_duration / 2)


def main(args=None):
    rclpy.init(args=args)
    led_updater = LEDUpdateNode()
    rclpy.spin(led_updater)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
