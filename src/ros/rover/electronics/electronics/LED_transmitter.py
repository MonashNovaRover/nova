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
from core.msg import RadioStatus, DriveInfo
from coms_utils.can_interface import CANTransmitter
from rclpy.qos import QoSProfile, QoSReliabilityPolicy
import time
from enum import Enum, IntEnum

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
    AUTONOMOUS = 2
    MANUAL = 3


class AutonomousState(Enum):
    ACTIVE = 4
    SUCCESS = 5


class LedColor(IntEnum):
    RED = 0x091
    GREEN = 0x092
    BLUE = 0x093


class CanLEDCommunicator:
    """
    Handles communication with LED
    """
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
        self.set_led(LedColor.RED, 0)
        self.set_led(LedColor.GREEN, 0)
        self.set_led(LedColor.BLUE, 0)


class LEDUpdateNode(Node):
    """class: its job is to check the status of the rover and update the LED via can on changes

    Note: this is a temporary approach. Really, we should be putting heartbeat as a mode
    in a topic with the other state types, and then this should be a subscriber to that topic.
    Then, a callback function will get executed periodically as the topic is published to. 
    """
    def __init__(self):
        super().__init__('LED_status_update_node')
        self.can_communicator = CanLEDCommunicator()

        self.connection_state = ConnectionState.DISCONNECTED
        self.control_state = ControlState.MANUAL
        self.autonomous_state = AutonomousState.ACTIVE

        # Subscriber to handle control state
        self.drive_info_subscriber = self.create_subscription(DriveInfo, "/control/drive_info", self.callback_drive_info, 10)
        
        # Subscriber to handle connection state
        # self.radio_subscriber = self.create_subscription(RadioStatus, "/electronics/radio_status", self.callback_connection, 10)

        # Services to handle autonomous state
        self.success_service = self.create_service(Trigger, 'autonomous/success', self.callback_auto_success)
        self.start_service = self.create_service(Trigger, 'autonomous/start', self.callback_auto_start)

        self.qos_time = 200

        self.flash_timer = self.create_timer(0.5, self.callback_flash)

        self.flash_counter = 1  # 1 = on, 0 = off

        self.most_recent_update = time.perf_counter()

        self.display()
        
    def do_flash(self):
        """
        Returns true if the LEDs should flash in the current state, otherwise false
        """
        if self.control_state == ControlState.AUTONOMOUS:
            if self.autonomous_state == AutonomousState.SUCCESS:
                # Always flash in Success state, to make sure we get points when we finish a task
                return True
            elif self.autonomous_state == AutonomousState.ACTIVE:
                # In autonomous mode our autonomous state determines whether we flash
                return False
        elif self.connection_state == ConnectionState.CONNECTED:
            # we are in manual mode, so flashing is determined by whether the controller is connected or not
            # Solid blue if we are connected in manual mode
            return False
        else:
            # Flash red if we have lost connection in manual mode
            return True

    def get_color(self):
        """
        Returns the LED colour and intensity [0, 255] to be displayed depending on the current state
        """
        if self.control_state == ControlState.AUTONOMOUS:
            if self.autonomous_state == AutonomousState.SUCCESS:
                # Always flash in Success state, to make sure we get points when we finish a task
                return LedColor.GREEN, 255
            else:
                # In autonomous mode we always display red
                return LedColor.RED, 255
        elif self.connection_state == ConnectionState.CONNECTED:
            # we are in manual mode, so color is determined by whether the controller is connected or not
            # Solid blue if we are connected in manual mode
            return LedColor.BLUE, 255
        else:
            # Flash red if we have lost connection in manual mode
            return LedColor.RED, 255

    def callback_connection(self, msg):
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

    def callback_drive_info(self, msg: DriveInfo):
        """
        callback that checks for the button B or A pressed on the controller and updates the state accordingly
        :param message: InputGamepad.msg type
        """
        # Check autonomous mode
        if msg.autonomous_mode:
            new_control_state = ControlState.AUTONOMOUS
        else:
            new_control_state = ControlState.MANUAL
        self.change_control_state(new_control_state)

        # Check connection
        if msg.connected:
            new_connection_state = ConnectionState.CONNECTED
        else:
            new_connection_state = ConnectionState.DISCONNECTED

        self.change_connection_state(new_connection_state)

    def callback_auto_success(self, request, response):
        response.success = True
        self.autonomous_state = AutonomousState.SUCCESS
        self.display()
        return response

    def callback_auto_start(self, request, response):
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

    def callback_flash(self):
        """
        Display a colour based on the mode of the rover either continuous or flashing
        """
        self.get_logger().debug("Flash callback")
        if not self.do_flash():
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
        colour_info = self.get_color()
        self.can_communicator.turn_off()
        self.can_communicator.set_led(*colour_info)


def main(args=None):
    rclpy.init(args=args)
    led_updater = LEDUpdateNode()
    rclpy.spin(led_updater)
    led_updater.destroy_node()
    rclpy.shutdown()


if __name__ == '__main__':
    main()
