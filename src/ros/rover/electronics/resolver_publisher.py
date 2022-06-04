#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 publisher code for the motor resolvers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: resolver_publisher
TOPICS:
  - /electronics/resolvers                [sensor_msgs/JointState]    [Published]
SERVICES:
  - /control/arm_config_info              [core/ArmConfigInfo]        [Client]
  - /electronics/resolver_zero_service    [core/StringTrigger]        [Server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     electronics 
AUTHOR(S):   Josh Cherubino, Jory Braun
CREATION:    14/02/2022
EDITED:      01/06/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
    - Setup appropriate QoS profile for publisher
    - Set appropriate transmit and receive timeouts
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""  

import rclpy
from rclpy.node import Node
from rclpy.impl.rcutils_logger import RcutilsLogger
from sensor_msgs.msg import JointState
from core.srv import ArmConfigInfo, StringTrigger

from coms_utils.uart_interface import UARTTransceiver
from math import pi
import time


class Joint:
    """
    Class to store joint-specific hardware information
    """
    def __init__(self, joint_name: str, id: int, reverse: bool=False, discontinuity_angle: float=2*pi, active: bool=False):
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

        # Bool for whether the joint is currently attached to the arm
        self.active = active


class ResolverTransceiver(UARTTransceiver):
    """
    Transceiver class to handle reading values from encoders
    """
    def __init__(self, logger: RcutilsLogger, **kwargs):
        super().__init__(**kwargs)

        # Set python logger level so will not log anything, add ROS logger
        self.set_log_level("critical")
        self.logger = logger
        
        # Create mapping of joint names to their respective Joint objects
        # Initialise using default discontinuity angles and active status,
        # update in the managing ROS node using info from the arm model
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

    def get_joint(self, joint_name: str, exclude_inactive: bool=True) -> Joint:
        """
        Return the Joint associated with the given joint name

        Raises KeyError if invalid joint name given
        By default, also raises KeyError if joint is not active
        """
        try:
            joint = self.joint_map[joint_name]
        except KeyError:
            # re raise with more useful message
            raise KeyError(f"Invalid joint name: {joint_name}")
        if exclude_inactive and not joint.active:
            raise KeyError(f"Inactive joint: {joint_name}")
        return joint

    def zero(self, joint_name: str) -> bool:
        """
        Method to zero a given encoder

        Returns True on success, false otherwise

        Raises KeyError if invalid joint name given
        """
        resolver_id = self.get_joint(joint_name).id
        
        self.logger.info(f'Zeroing joint {joint_name}')
        # Send two bytes, so use 2-byte format
        # First byte is resolver_id + 0x02. Indicates an extended command
        # Second byte is zero command: 0x5E
        data = self.pack([resolver_id + 0x02, 0x5E], fmt='<BB')
        transmitted = self.transmit(data)
        if not transmitted:
            self.logger.error(f'Transmit timeout for joint {joint_name}')
        return transmitted
    
    def position(self, joint_name: str) -> float:
        """
        Method to read a given encoder
        
        Returns float value in [0, 2 pi) or -1 on failure

        Raises KeyError if invalid joint name given
        """
        joint = self.get_joint(joint_name)

        resolver_id = joint.id

        # Pack and transmit binary data
        data = self.pack([resolver_id])
        if not self.transmit(data):
            self.logger.error(f'Transmit timeout for joint {joint_name}')
            return -1

        # Read response and decode into radians
        # Receives two bytes from the resolvers, representing a single 16-bit value
        bytes_data = self.receive()
        if bytes_data is None:
            self.logger.error(f'Read timeout for joint {joint_name}')
            return -1
        integer_data = self.unpack(bytes_data)[0]
        # Handle checksum
        if not self._verify_checksum(integer_data):
            self.logger.warn(f'Invalid checksum from joint {joint_name}')
            return -1
        # Get angle data by removing 2 high order bits
        angle_data = self._convert_to_rad(integer_data & 0x3FFF)
        
        # Reverse the increasing direction if necessary
        if joint.reverse:
            angle_data = self._reverse_direction(angle_data)
    
        # Shift the angle discontinuity out of each joint's range of motion
        angle_data = self._move_discontinuity(angle_data, joint.discontinuity_angle)

        return angle_data

    @staticmethod
    def _verify_checksum(raw_value: int) -> bool:
        """
        Verifies the checksum for CUI Devices AMT21 absolute encodrs

        The data is 16-bits long
        Valid data has odd parity for all the even bits, and for all the odd bits.
        Bits are numbered from 0 starting with the LSB.
        """ 
        assert raw_value < 65536
        binary_data = [int(bit) for bit in f"{raw_value:016b}"]
        even_bits = [binary_data[i] for i in range(len(binary_data)) if i % 2]
        odd_bits = [binary_data[i] for i in range(len(binary_data)) if not i % 2]
        return sum(even_bits) % 2 and sum(odd_bits) % 2

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
            transmit_fmt = '<B',
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

        # Update info for Joint objects in the ResolverTransceiver
        for i, joint_name in enumerate(joint_names):
            joint = self.resolver_transceiver.get_joint(joint_name, exclude_inactive=False)
            
            # Set the used joints to active
            joint.active = True

            # Store the discontinuity angles
            joint_limit_lower = self.arm_config_info.joint_limits_lower[i]
            joint_limit_upper = self.arm_config_info.joint_limits_upper[i]
            joint.discontinuity_angle = self.wrap_to_2pi((joint_limit_lower + joint_limit_upper) / 2 + pi)

        # Construct and start the resolver publisher
        self.publisher = self.create_publisher(JointState, '/electronics/resolvers', 10)
        self.resolver_pub_timer = self.create_timer(resolver_pub_timer_period, self.publish)
        # Construct the service to zero resolvers
        self.zero_service = self.create_service(StringTrigger, "/electronics/resolver_zero_service", self.zero_callback)

    def zero_callback(self, request: StringTrigger.Request, response: StringTrigger.Response):
        """
        Callback for the resolver zero service
        """
        joint_name = request.value
        try:
            response.success = self.resolver_transceiver.zero(joint_name)
            if response.success:
                response.message = f"Successfully transmitted data for joint {joint_name}"
            else:
                response.message = f"Transmit timeout requesting data from joint {joint_name}"
        except KeyError as e:
            response.success = False
            response.message = str(e).replace("'", "")
        return response

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
        for i, joint_name in enumerate(self.arm_config_info.joint_names):

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

    rclpy.spin(resolver_pub)

    # Destroy the node explicitly
    resolver_pub.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
