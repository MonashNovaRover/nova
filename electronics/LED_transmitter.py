#!/usr/bin/python3

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
from std_srvs.srv import Trigger
from core.msg import InputGamepad
from coms_utils.can_interface import CANTransmitter
from rclpy.qos import QoSProfile, QoSReliabilityPolicy
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
    RED = 0x091
    GREEN = 0x092
    BLUE = 0x093

    def __init__(self):
        self.transmitter = CANTransmitter(
            channel='can0',  # Can channel to transmit on
            arbitration_id=0x091,  # ID for red can trasmitter
        )

    def set_LED(self, colour, intensity):
        """
        Sends data to CAN bus to actually activate the LED array
        """
        self.transmitter.arbitration_id = colour  # send to the desired colour LED
        packed_data = int.to_bytes(intensity, 1, "big")
        ret = self.transmitter.transmit(packed_data)

        return ret  # for informing of errors

    def turn_off(self, colour):
        """
        Tells a given colour line to display 0 intensity
        """
        self.set_LED(colour, 0)


class LEDUpdateNode(Node):
    """class: its job is to check the status of the rover and update the LED via can on changes

    Note: this is a temporary approach. Really, we should be putting heartbeat as a mode
    in a topic with the other mode types, and then this should be a subscriber to that topic.
    Then, a callback function will get executed periodically as the topic is published to. 
    """
    MANUAL = 0
    AUTONOMOUS = 1
    AUTO_GOAL_ACHIEVED = 2
    DISCONNECTED = 3


    def __init__(self):
        super().__init__('LED_status_update_node')
        self.can_communicator = CanLEDCommunicator()

        best_effort = QoSReliabilityPolicy.RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT
        qos = QoSProfile(reliability=best_effort, depth=10)

        self.gamepad_input_subscriber = self.create_subscription(InputGamepad, "/control/input_gamepad", self.gamepad_callback, qos)

        self.service = self.create_service(Trigger, '/autonomous/LED', self.service_callback)
        self.qos_time = 0.1

        self.qos_timer = self.create_timer(self.qos_time, self.check_connection)
        self.flash_timer = self.create_timer(0.5, self.flash_callback)

        # Colours and their brightness for each mode (mostly half brightness to save power)
        self.mode_colours = {
            LEDUpdateNode.MANUAL: (CanLEDCommunicator.BLUE, 128),
            LEDUpdateNode.AUTONOMOUS: (CanLEDCommunicator.RED, 128),
            LEDUpdateNode.AUTO_GOAL_ACHIEVED: (CanLEDCommunicator.GREEN, 128),
            LEDUpdateNode.DISCONNECTED: (CanLEDCommunicator.RED, 255)
        }

        # Control how the above colour is displayed
        self.mode_flash = {
            LEDUpdateNode.MANUAL: False,
            LEDUpdateNode.AUTONOMOUS: False,
            LEDUpdateNode.AUTO_GOAL_ACHIEVED: True,
            LEDUpdateNode.DISCONNECTED: True
        }

        self.flash_counter = 1  # 1 == on, 0 == off

        self.most_recent_update = time.perf_counter()

        self.mode = LEDUpdateNode.MANUAL
        self.previous_mode = self.mode

        self.display()

    def check_connection(self):
        """
        simple method called every second to check that the node has still
        been receiving Gamepad messages
        """
        dt = time.perf_counter() - self.most_recent_update

        new_mode = self.mode

        if dt > self.qos_time:
            new_mode = LEDUpdateNode.DISCONNECTED
        elif self.mode == LEDUpdateNode.DISCONNECTED:
            new_mode = LEDUpdateNode.MANUAL

        self.change_mode(new_mode)

    def gamepad_callback(self, message):
        """
        callback that checks for the button B or A pressed on the controller and updates the state accordingly
        :param message: InputGamepad.msg type
        """
        B = message.btn_b_state
        A = message.btn_a_state

        new_mode = self.mode
        if (B == 1 or B == 2):  # B is pressed or held
            new_mode = LEDUpdateNode.MANUAL
        elif (A == 1 or A == 2):  # A is pressed or held
            new_mode = LEDUpdateNode.AUTONOMOUS

        if message.connected:
            self.most_recent_update = time.perf_counter()
            if new_mode != self.mode:
                self.change_mode(new_mode)

    def service_callback(self, request, response):
        response.success = True
        self.change_mode(LEDUpdateNode.AUTO_GOAL_ACHIEVED)
        return response

    def change_mode(self, new_mode):
        if new_mode != self.mode:
            self.previous_mode = self.mode
            self.mode = new_mode
            self.display()

    def flash_callback(self):
        """
        Display a colour based on the mode of the rover either continuous or flashing
        """
        if not self.mode_flash[self.mode]: 
            return   # don't care about non-flashing modes

        if self.flash_counter == 0: 
            self.can_communicator.turn_off(self.mode_colours[self.mode][0])
        else:
            self.display()
        
        self.flash_counter = (self.flash_counter + 1) % 2

    def display(self):
        """
        Displays a colour based on the current mode of the Rover
        """
        # get colour and brightness
        colour_info = self.mode_colours[self.mode]
        self.can_communicator.turn_off(CanLEDCommunicator.RED)
        self.can_communicator.turn_off(CanLEDCommunicator.GREEN)
        self.can_communicator.turn_off(CanLEDCommunicator.BLUE)
        self.can_communicator.set_LED(*colour_info)


def main(args=None):
    rclpy.init(args=args)
    led_updater = LEDUpdateNode()
    rclpy.spin(led_updater)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
