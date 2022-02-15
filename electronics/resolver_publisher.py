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

from coms_utils.uart_interface import UARTTransceiver

class ResolverTransceiver(UARTTransceiver):
    '''
    Transceiver class to handle reading values from encoders
    '''
    def __init__(self, **kwargs):
        super().__init__(receive_timeout=.05, **kwargs)
        # create mapping of joint names to resolver ids for sending commands
        # FIXME: Based off the old script, the base IDs were 0x4, 0x8, 0xC, 0x10, 0x14
        # but unsure what the additional joints IDs are. Relatively easy to add them later though
        self.joint_id_map =  {
                "base-rotation":    0x04,
                "shoulder":         0x08,
                "elbow":            0x0C,
                "j4":               0x10, 
                "j5":               0x14, 
                "j6":               0x18
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

    def position(self, joint: str) -> float:
        '''
        Method to read a given encoder
        
        Returns float value in [0, 2 pi] or -1 on failure
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
        ret = self.receive()
        if ret is None:
            return -1
        unpacked_data = self.unpack(ret)[0]

        # TODO: Handle checksum 
        # for now just mask it out by removing 2 high order bits
        return self._convert_to_rad(unpacked_data & 0x3FFF)

    def _convert_to_rad(self, raw_value: int) -> float:
        '''
        Internal helper method to convert to radians
        '''
        # value will be between 0 and max 14-bit value 0x3FFF
        return raw_value/0x3FFF * 2*pi

class ResolverPublisher(Node):
    def __init__(self):
        super().__init__('resolver_publisher', start_parameter_services=False)
        self._publisher = self.create_publisher(JointState, '/electronics/resolvers', 10)
        timer_period = 0.5  # TODO: Adjust as needed
        self.timer = self.create_timer(timer_period, self.publish)

        self.resolver_transceiver = ResolverTransceiver(
                receive_fmt = '<H', # TODO: check this data format
                transmit_fmt = '@B', # TODO: check this data format
                logger = self.get_logger(),
                baudrate = 115200, # probably right?
                port = '/dev/ttyUSB0', # TODO: check this
                )

    def publish(self):
        '''
        callback to publish position of all joints
        '''
        msg = JointState()
        for joint in self.resolver_transceiver.joint_id_map.keys():
            msg.name.append(joint)
            msg.position.append(self.resolver_transceiver.position(joint))
        
        self._publisher.publish(msg)

    def destroy_node(self):
        '''
        Simple override to close comms before continuing with node destruction
        '''
        self.resolver_transceiver.close()
        return super().destroy_node()

def main(args=None):
    rclpy.init(args=args)

    resolver_pub = ResolverPublisher()

    rclpy.spin(resolver_pub)

    # Destroy the node explicitly
    resolver_pub.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()

