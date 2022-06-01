#!/usr/bin/python3

"""
NOVA ROVER TEAM
This script is a ros service that handles communicating commands to the LED Lights.
Authors: Max Tory and Marcel Masque
Last Modified: 14/04/2022 By Max Tory

TODO: Add custom routines that do different light patterns and sequences - maybe we
want to flash if disconnected etc. Gives us more options since we can't combine
colours
"""

import rclpy
from rclpy.node import Node
from std_srvs.srv import Trigger
from std_msgs.msg import Bool
from core.msg import RadioStatus
from coms_utils.can_interface import CANTransmitter
from rclpy.qos import QoSProfile, QoSReliabilityPolicy
import time
from enum import Enum

"""
Goal 1: get a request in the service, and execute
a callback function setting the colours.
     - for now is printed out to screen 
     - can bus not implemented

"""


class ConnectionState(Enum):
    CONNECTED = 0
    DISCONNECTED = 1


class ControlState(Enum):
    AUTONOMOUS = 0
    MANUAL = 1


class AutonomousState(Enum):
    ACTIVE = 0
    SUCCESS = 1


class CanLEDCommunicator:
    """
    Handles communication with LED
    """
    RED = 0x091
    GREEN = 0x092
    BLUE = 0x093

    def __init__(self):
        self.transmitter = CANTransmitter(
            channel='can0',  # Can channel to transmit on
            arbitration_id=0x091,  # ID for red can trasmitter
        )

    def set_led(self, colour, intensity):
        """
        Sends data to CAN bus to actually activate the LED array
        """
        self.transmitter.arbitration_id = colour  # send to the desired colour LED
        packed_data = int.to_bytes(intensity, 1, "big")
        ret = self.transmitter.transmit(packed_data)

        return ret  # for informing of errors

    def turn_off(self):
        """
        Tells a given colour line to display 0 intensity
        """
        self.set_led(CanLEDCommunicator.RED, 0)
        self.set_led(CanLEDCommunicator.GREEN, 0)
        self.set_led(CanLEDCommunicator.BLUE, 0)


class LEDUpdateNode(Node):
    """class: its job is to check the status of the rover and update the LED via can on changes

    Note: this is a temporary approach. Really, we should be putting heartbeat as a mode
    in a topic with the other state types, and then this should be a subscriber to that topic.
    Then, a callback function will get executed periodically as the topic is published to. 
    """
    def __init__(self):
        super().__init__('LED_status_update_node')
        self.can_communicator = CanLEDCommunicator()

        self.connection_state = ConnectionState.CONNECTED
        self.control_state = ControlState.MANUAL
        self.autonomous_state = AutonomousState.ACTIVE

        # Subscriber to handle control state
        self.mode_subscriber = self.create_subscription(Bool, "/autonomous/mode", self.mode_callback, 10)
        self.radio_subscriber = self.create_subscription(RadioStatus, "/electronics/radio_status", self.connection_callback, 10)

        # Services to switch in and out of success mode
        self.success_service = self.create_service(Trigger, 'autonomous/success', self.success_callback)
        self.start_service = self.create_service(Trigger, 'autonomous/start', self.start_callback)

        best_effort = QoSReliabilityPolicy.RMW_QOS_POLICY_RELIABILITY_BEST_EFFORT
        qos = QoSProfile(reliability=best_effort, depth=10)

        self.qos_time = 0.3

        self.flash_timer = self.create_timer(0.5, self.flash_callback)

        # Colours and their brightness for each mode (mostly half brightness to save power)
        self.control_state_colours = {
            ControlState.MANUAL: (CanLEDCommunicator.BLUE, 255),
            ControlState.AUTONOMOUS: (CanLEDCommunicator.RED, 255),
        }

        # Control how the above colour is displayed
        self.control_state_flash = {
            ControlState.MANUAL: False,
            ControlState.AUTONOMOUS: False,
        }

        self.connection_state_colours = {
            ConnectionState.CONNECTED: self.control_state_colours[self.control_state],
            ConnectionState.DISCONNECTED: (CanLEDCommunicator.RED, 255)
        }

        self.connection_state_flash = {
            ConnectionState.CONNECTED: self.control_state_flash[self.control_state],
            ConnectionState.DISCONNECTED: True
        }

        self.state_colours = {
            AutonomousState.ACTIVE: self.connection_state_colours[self.connection_state],
            AutonomousState.SUCCESS: (CanLEDCommunicator.GREEN, 255)
        }

        self.state_flash = {
            AutonomousState.ACTIVE: self.connection_state_flash[self.connection_state],
            AutonomousState.SUCCESS: True
        }

        self.flash_counter = 1  # 1 = on, 0 = off

        self.most_recent_update = time.perf_counter()

        self.display()

    def connection_callback(self, msg):
        """
        simple method called every second to check that the node has still
        been receiving Gamepad messages
        """
        ping = msg.ping

        new_connection_state = self.connection_state
        if ping > self.qos_time:
            new_connection_state = ConnectionState.DISCONNECTED
        else:
            new_connection_state = ConnectionState.CONNECTED

        self.change_connection_state(new_connection_state)

    def mode_callback(self, message):
        """
        callback that checks for the button B or A pressed on the controller and updates the state accordingly
        :param message: InputGamepad.msg type
        """
        new_control_state = self.control_state
        if message.data:
            new_control_state = ControlState.AUTONOMOUS
        else:  
            new_control_state = ControlState.MANUAL

        self.change_control_state(new_control_state)

    def success_callback(self, request, response):
        response.success = True
        self.autonomous_state = AutonomousState.SUCCESS
        self.display()
        return response

    def start_callback(self, request, response):
        response.success = True
        self.autonomous_state = AutonomousState.ACTIVE
        self.display()
        return response

    def change_connection_state(self, new_connection_state):
        if new_connection_state != self.connection_state:
            self.connection_state = new_connection_state
            self.display()

    def change_control_state(self, new_control_state):
        if new_control_state != self.control_state:
            self.control_state = new_control_state
            self.display()

    def flash_callback(self):
        """
        Display a colour based on the mode of the rover either continuous or flashing
        """
        if not self.state_flash[self.autonomous_state]:
            return   # don't care about non-flashing modes

        if self.flash_counter == 0: 
            self.can_communicator.turn_off()
        else:
            self.display()
        
        self.flash_counter = (self.flash_counter + 1) % 2

    def display(self):
        """
        Displays a colour based on the current mode of the Rover
        """
        # get colour and brightness
        colour_info = self.state_colours[self.autonomous_state]
        self.can_communicator.turn_off()
        self.can_communicator.set_led(*colour_info)


def main(args=None):
    rclpy.init(args=args)
    led_updater = LEDUpdateNode()
    rclpy.spin(led_updater)
    rclpy.shutdown()


if __name__ == '__main__':
    main()
