#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 publisher code for the motor resolvers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     electronics 
AUTHOR(S):    Josh Cherubino
CREATION:    14/02/2022
EDITED:      14/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
    - Add checksum validation of data
    - Add subscription to topic to see what joints are connected
    - Setup appropriate QoS profile
    - Set appropriate transmit and receive timeouts
    - Check structure of unpacked_data - should it be a tuple? Only the 0th element has stuff in it.
    - Add ROS service for zeroing resolvers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""  

from math import pi
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState
import time

from coms_utils.uart_interface import UARTTransceiver

class ResolverTransceiver(UARTTransceiver):
    '''
    Transceiver class to handle reading values from encoders
    '''
    def __init__(self, **kwargs):
        super().__init__(receive_timeout=.05, **kwargs)
        # create mapping of joint names to resolver ids for sending commands
        self.joint_id_map =  {
            "base-rotation":    0x04,
            "shoulder":         0x08,
            "elbow":            0x0C,
            "j4":               0x10,
            "j5":               0x14,
            "j6":               0x18
        }

        # Create mapping of joint names where the resolver angle increases in the wrong direction
        # For a given joint, a value of 1 means the direction needs to be flipped.
        self.joint_direction_map = {
            "base-rotation":    1,
            "shoulder":         1,
            "elbow":            0,
            "j4":               0,
            "j5":               0,
            "j6":               0
        }

        # Create mapping of joint names to desired angular discontinuity angle
        # Set the discontinuity outside the range of motion of each joint
        self.discontinuity_angle_map = {
            "base-rotation":    pi,
            "shoulder":         pi,
            "elbow":            pi,
            "j4":               pi,
            "j5":               pi,
            "j6":               0
        }

    def zero(self, joint: str) -> bool:
        '''
        Method to zero a given encoder

        Returns True on success, false otherwise

        Raises KeyError if invalid joint given
        '''
        try:
            resolver_id = self.joint_id_map[joint]
        except KeyError as e:
            # re raise with more useful message
            raise KeyError(f"Invalid joint name: {joint}")
        
        self.info(f'Zeroing joint {joint}')
        # format (first byte is begin extended command: 0x56)
        # second is zero command: 0x5E
        fmt = '@BB'
        data = self.pack([0x56, 0x5E], fmt=fmt)
        return self.transmit(data)

    @staticmethod
    def reverse_direction(angle: float) -> float:
        '''
        Method to reverse the increasing direction of a resolver

        Maps [0, 2pi) to (2pi, 0]
        '''
        if angle != 0:
            angle = 2*pi - angle
        return angle
    
    def position(self, joint: str) -> float:
        '''
        Method to read a given encoder
        
        Returns float value in [0, 2 pi) or -1 on failure
        '''
        try:
            resolver_id = self.joint_id_map[joint]
        except KeyError as e:
            # re raise with more useful message
            raise KeyError(f"Invalid joint name: {joint}")
        
        # pack and transmit binary data
        data = self.pack([resolver_id])
        if not self.transmit(data):
            return -1

        # read response and decode into radians
        ret = self.receive(error_string=f"Device ID {hex(resolver_id)}")
        if ret is None:
            return -1
        unpacked_data = self.unpack(ret)[0]
        # TODO: Handle checksum 
        # for now just mask it out by removing 2 high order bits
        angle_data = self._convert_to_rad(unpacked_data & 0x3FFF)

        # Shift the angle discontinuity out of each joint's range of motion
        angle_data = self._move_discontinuity(angle_data, self.discontinuity_angle_map[joint])

        # Reverse the increasing direction if necessary
        if self.joint_direction_map[joint]:
            angle_data = self.reverse_direction(angle_data)
        return angle_data

    def _convert_to_rad(self, raw_value: int) -> float:
        '''
        Internal helper method to convert to radians
        '''
        # value will be between 0 and max 14-bit value 0x3FFF
        return raw_value/0x3FFF * 2*pi

    @staticmethod
    def _move_discontinuity(angle: float, discontinuity_angle: float) -> float:
        '''
        Move the periodic angle discontinuity from 2pi to some specifcied angle
        '''
        return angle - 2*pi * (angle >= discontinuity_angle)

class ResolverPublisher(Node):
    def __init__(self):
        super().__init__('resolver_publisher', start_parameter_services=False)
        self._publisher = self.create_publisher(JointState, '/electronics/resolvers', 10)
        timer_period = 0.5  # TODO: Adjust as needed
        self.timer = self.create_timer(timer_period, self.publish)

        self.resolver_transceiver = ResolverTransceiver(
                receive_fmt = '<H',
                transmit_fmt = '@B',
                logger = self.get_logger(),
                baudrate = 115200,
                port = '/dev/ttyUSB0',
                )

        # Create the output message type to track the resolver state
        self.resolver_state = JointState()
        joint_names = self.resolver_transceiver.joint_id_map.keys()
        self.resolver_state.name = joint_names
        self.resolver_state.position = [0.0] * len(joint_names)
        # Initialise unused fields to have correct lengths for consistency
        self.resolver_state.velocity = [0.0] * len(joint_names)
        self.resolver_state.effort = [0.0] * len(joint_names)

    def publish(self):
        '''
        callback to publish position of all joints
        '''
        for i, joint_name in enumerate(self.resolver_state.name):

            # Do not check J6, just pretend it is always level
            if joint_name == "j6":
                time.sleep(0.05)
                continue

            joint_position = self.resolver_transceiver.position(joint_name)
            if joint_position != -1:
                # Successful transmit and receive, publish new value
                self.resolver_state.position[i] = joint_position
        
        self._publisher.publish(self.resolver_state)

    def destroy_node(self):
        '''
        Simple override to close comms before continuing with node destruction
        '''
        self.resolver_transceiver.close()
        return super().destroy_node()

def main(args=None):
    rclpy.init(args=args)

    resolver_pub = ResolverPublisher()
    #resolver_pub.resolver_transceiver.zero("elbow")

    rclpy.spin(resolver_pub)

    # Destroy the node explicitly
    resolver_pub.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()

