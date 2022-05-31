#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 publisher code for the motor resolvers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     electronics 
AUTHOR(S):   Josh Cherubino, Jory Braun
CREATION:    14/02/2022
EDITED:      28/04/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
    - Add checksum validation of data
    - Setup appropriate QoS profile for publisher
    - Set appropriate transmit and receive timeouts
    - Check structure of unpacked_data - should it be a tuple? Only the 0th element has stuff in it.
    - Add ROS service for zeroing resolvers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""  

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import JointState
from core.srv import ArmConfigInfo

from coms_utils.uart_interface import UARTTransceiver
from math import pi
import time


class Joint:
    """
    Class to store joint-specific hardware information
    """
    def __init__(self, joint_name: str, id: int, reverse: bool=False, discontinuity_angle: float=2*pi):
        # Joint names as in the arm model
        self.joint_name = joint_name
        # Resolver ID for sending commands
        self.id = id
        
        # Bool for whether the resolver angle increases in the wrong direction
        # The joint-angle positive direction is defined by the DH convention
        # If the direction needs to be flipped, store True, otherwise False.
        self.reverse = reverse
        
        # Resolver readings are in the range [0, 2pi), and there is a discontinuity once the angle grows to 2pi
        # Move the discontinuity to some angle outside the normal range of joint motion
        # Makes the joint limits calculation much simpler
        self.discontinuity_angle = discontinuity_angle


class ResolverTransceiver(UARTTransceiver):
    """
    Transceiver class to handle reading values from encoders
    """
    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        
        # Create mapping of joint names to their respective Joint objects
        self.joint_map =  {
            "base-rotation":    Joint("base-rotation", 0x04, True),
            "shoulder":         Joint("shoulder",      0x08, True),
            "elbow":            Joint("elbow",         0x0C, False),
            "j4":               Joint("j4",            0x10, False),
            "j5":               Joint("j5",            0x14, False),
            "j6":               Joint("j6",            0x18, False),
            "spmx":             Joint("spmx",          0x20, True),
            "spmy":             Joint("spmy",          0x24, True),
            "spmz":             Joint("spmz",          0x28, True),
            "end-rotation":     Joint("end-rotation",  0x1C, False)
        }

    def zero(self, joint_name: str) -> bool:
        """
        Method to zero a given encoder

        Returns True on success, false otherwise

        Raises KeyError if invalid joint given
        """
        try:
            resolver_id = self.joint_map[joint_name].id
        except KeyError as e:
            # re raise with more useful message
            raise KeyError(f"Invalid joint name: {joint_name}")
        
        self.info(f'Zeroing joint {joint_name}')
        # Send two bytes, so use 2-byte format
        # First byte is resolver_id + 0x02. Indicates an extended command
        # Second byte is zero command: 0x5E
        data = self.pack([resolver_id + 0x02, 0x5E], fmt='<BB')
        return self.transmit(data)
    
    def position(self, joint_name: str) -> float:
        """
        Method to read a given encoder
        
        Returns float value in [0, 2 pi) or -1 on failure
        """
        try:
            joint = self.joint_map[joint_name]
        except KeyError as e:
            # re raise with more useful message
            raise KeyError(f"Invalid joint name: {joint_name}")
        
        resolver_id = joint.id

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
        
        # Reverse the increasing direction if necessary
        if joint.reverse:
            angle_data = self._reverse_direction(angle_data)
    
        # Shift the angle discontinuity out of each joint's range of motion
        angle_data = self._move_discontinuity(angle_data, joint.discontinuity_angle)

        return angle_data

    @staticmethod
    def _convert_to_rad(raw_value: int) -> float:
        """
        Internal helper method to convert to radians
        """
        # value will be between 0 and max 14-bit value 0x3FFF
        return raw_value/0x3FFF * 2*pi

    @staticmethod
    def _reverse_direction(angle: float) -> float:
        """
        Reverse the increasing direction of a resolver

        Maps [0, 2pi) to (2pi, 0]
        """
        if angle != 0:
            angle = 2*pi - angle
        return angle
    
    @staticmethod
    def _move_discontinuity(angle: float, discontinuity_angle: float) -> float:
        """
        Move the periodic angle discontinuity from 2pi to some specifcied angle
        """
        return angle - 2*pi * (angle >= discontinuity_angle)


class ResolverPublisher(Node):
    def __init__(self):
        """
        Start the node and make a service request to /control/arm_config_info
        """
        super().__init__('resolver_publisher', start_parameter_services=False)
                
        # Create the client for /control/arm_config_info
        self.client = self.create_client(ArmConfigInfo, "/control/arm_config_info")
        # Wait for the service to become available
        while not self.client.wait_for_service(timeout_sec=1.0):
            self.get_logger().info("Service /control/arm_config_info not available, waiting again...")
        # Make the service request
        request = ArmConfigInfo.Request()
        self.future = self.client.call_async(request)

        # Set up the callback timer
        self.client_check_timer = self.create_timer(0.1, self.client_check_callback)

    def client_check_callback(self):
        """
        Check if /control/arm_config_info has responded. If so, save the data and set up the node
        """
        if self.future.done():
            # Got a response!
            self.get_logger().info("Got a response from /control/arm_config_info. Starting the node.")
            self.client_check_timer.cancel()
            self.arm_config_info = self.future.result()
            self.start_node()
        else:
            self.get_logger().info("Failed to get response from /control/arm_config_info, waiting again...")
    
    def start_node(self):
        """
        Setup the node for the application. Create pubs and subs, initialise data members
        """
        self.receive_timeout = 0.05
        resolver_pub_timer_period = 0.5
        
        # Initialise the transceiver
        self.resolver_transceiver = ResolverTransceiver(
                receive_timeout = self.receive_timeout,
                receive_fmt = '<H',
                transmit_fmt = '@B',
                logger = self.get_logger(),
                baudrate = 115200,
                port = '/dev/ttyUSB0',
                )

        # Create the output message type to track the resolver state
        self.resolver_state = JointState()
        joint_names = self.arm_config_info.joint_names
        self.resolver_state.name = joint_names
        self.resolver_state.position = [0.0] * len(joint_names)
        # Initialise unused fields to have correct lengths for consistency
        self.resolver_state.velocity = [0.0] * len(joint_names)
        self.resolver_state.effort = [0.0] * len(joint_names)

        # Store the discontinuity angles for Joint objects in the ResolverTransceiver
        for i, joint_name in enumerate(joint_names):
            try:
                joint = self.resolver_transceiver.joint_map[joint_name]
            except KeyError as e:
                # re raise with more useful message
                raise KeyError(f"Invalid joint name: {joint_name}")
            
            joint_limit_lower = self.arm_config_info.joint_limits_lower[i]
            joint_limit_upper = self.arm_config_info.joint_limits_upper[i]
            joint.discontinuity_angle = self.wrap_to_2pi((joint_limit_lower + joint_limit_upper) / 2 + pi)

        # Construct and start the resolver publisher
        self.publisher = self.create_publisher(JointState, '/electronics/resolvers', 10)
        self.resolver_pub_timer = self.create_timer(resolver_pub_timer_period, self.publish)

    @staticmethod
    def wrap_to_2pi(angle: float) -> float:
        """
        Convert a Real angle into the equivalent angle in [0, 2pi)
        """
        angle = angle % (2*pi)
        if angle < 0:
            angle += 2*pi
        return angle
    
    def publish(self):
        """
        callback to publish position of all joints
        """
        for i, joint_name in enumerate(self.resolver_state.name):

            # No resolver on J6, so just pretend it is always level
            if joint_name == "j6":
                # Delay a little to not overwhelm the RS485 bus
                time.sleep(self.receive_timeout)
                continue

            joint_position = self.resolver_transceiver.position(joint_name)
            if joint_position != -1:
                # Successful transmit and receive, update value to be published
                self.resolver_state.position[i] = joint_position                
        
        self.publisher.publish(self.resolver_state)

    def destroy_node(self):
        """
        Simple override to close comms before continuing with node destruction
        """
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
