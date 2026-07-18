#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS Node for maintaining/updating Raman 
mechanical state
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: raman_mech_server
TOPICS: 
    - /science/raman_mech_msg [RamanState] [Publisher]
SERVICES: 
    - /science/raman_mech_srv [RamanMech] [Server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR:         Connor Macdougall
CREATION:	    18/01/2024
EDITED:		    22/05/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
MORE INFO:
 - https://www.notion.so/Raman-Spectra-0161f5611e934a779247f3733ca8a608
"""

import logging
import rclpy
from rclpy.node import Node
import jcan

from science_interfaces.msg import RamanState
from science_interfaces.srv import RamanMech


class RamanMechServer(Node):
    # CAN commands
    CARD_ID = 0x0A0
    DISABLE_CARD = 0x00
    GREEN_LASER_ID = 0x01
    RED_LASER_ID = 0x02
    MIRROR_SERVO_ID = 0x03
    FILTER_SERVO_ID = 0x04
    STEPPER_ID = 0x05
    CAN_COMMAND_ON = 0x01
    CAN_COMMAND_OFF = 0x00

    # ROS params
    CAN_BUS_PARAM = "can_bus"

    # ROS mech channels
    MECH_STATE_SERVICE = '/science/raman_mech_srv'
    MECH_STATE_TOPIC = '/science/raman_mech_msg'

    # initial state values
    DEFAULT_MECH = False, False, 0.0, 0.0, 0.0             # A tuple of 5 values (greenlaseron, redlaseron, filterselection, steppervalue and mirrorservo, in that order)


    def __init__(self):
        super().__init__('raman_mech_server')
        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Raman Mech Server starting")

        self.declare_parameter(RamanMechServer.CAN_BUS_PARAM, "can1")

        # initialising node values 
        self.mech_state = RamanMechServer.DEFAULT_MECH

        # for mechanical
        self.mech_srv = self.create_service(RamanMech, RamanMechServer.MECH_STATE_SERVICE, self.raman_mech_response)
        self.mech_publisher_ = self.create_publisher(RamanState, RamanMechServer.MECH_STATE_TOPIC, 10)
        self.timer_publish_state = self.create_timer(1, self.publish_state)
        self.timer_send_can_commands = self.create_timer(0.2, self.send_can_commands)

        # for CAN commands
        self.bus = jcan.Bus()
        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)
        self.timer_spin_can = self.create_timer(0.05, self.bus.spin)


    def send_can_commands(self):
        """
        Sends all can commands to update mechanical state to what is current 
        """
        self.send_laser_command()
        self.send_filter_command()
        self.send_stepper_command()
        self.send_mirror_command()

    def send_can_command(self, commands):
        for command in commands:
            mech_frame = jcan.Frame(RamanMechServer.CARD_ID, command)
            self.bus.send(mech_frame)


    def send_laser_command(self):
        if self.mech_state[0]:
            self.send_can_command([[RamanMechServer.GREEN_LASER_ID, RamanMechServer.CAN_COMMAND_ON], [RamanMechServer.RED_LASER_ID, RamanMechServer.CAN_COMMAND_OFF]])
        elif self.mech_state[1]:
            self.send_can_command([[RamanMechServer.GREEN_LASER_ID, RamanMechServer.CAN_COMMAND_OFF], [RamanMechServer.RED_LASER_ID, RamanMechServer.CAN_COMMAND_ON]])
        else:
            self.send_can_command([[RamanMechServer.GREEN_LASER_ID, RamanMechServer.CAN_COMMAND_OFF], [RamanMechServer.RED_LASER_ID, RamanMechServer.CAN_COMMAND_OFF]])

    def send_filter_command(self):
        self.send_can_command([[RamanMechServer.FILTER_SERVO_ID, self.mech_state[2]]])

    def send_stepper_command(self):
        self.send_can_command([[RamanMechServer.STEPPER_ID, self.mech_state[3]]])

    def send_mirror_command(self):
        self.send_can_command([[RamanMechServer.MIRROR_SERVO_ID, self.mech_state[4]]]) 


    def raman_mech_response(self, request, response):
        """
        Updates the mechanical state that the node will send can commands based on
        """
        self.mech_state = request.green_laser_on, request.red_laser_on, request.filter_selection, request.stepper_value, request.mirror_servo
        response.success = True
        self.get_logger().info(f"Raman Mechanical State updated")
        return response


    def publish_state(self):
        """
        Publishes raman mechanical state
        """
        msg = RamanState()
        msg.green_laser_on, msg.red_laser_on, msg.filter_selection, msg.stepper_value, msg.mirror_servo = self.mech_state
        self.mech_publisher_.publish(msg)


def main(args=None):
    rclpy.init(args=args)
    server = RamanMechServer()
    rclpy.spin(server)
    server.destroy_node()
    rclpy.shutdown()
    
if __name__ == "__main__":
    main()
