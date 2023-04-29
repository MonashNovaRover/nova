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
AUTHOR(S):   Jory Braun, Tom Newton, Josh Cherubino
CREATION:    14/02/2022
EDITED:      21/03/2023
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

from coms_utils.can_interface import CANTransceiver
from math import pi
from struct import calcsize
import time


class Joint:
    """
    Class to store joint-specific hardware information
    """
    def __init__(self, joint_name: str, id: int, reverse: bool=False, discontinuity_angle: float=2*pi, gear_ratio: int=1, active: bool=False):
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

        # Parameters for resolvers with a geared connection to a joint
        # Gear ratio between actual joint and the resolver (resolver turns per joint turn)
        self.gear_ratio = gear_ratio
        # Previous angle reading of the resolver
        self.last_reading = None
        # Track which sector the joint is in on the actual joint
        # The size of each sector is 2*pi/gear_ratio
        self.sector_count = 0

        # Bool for whether the joint is currently attached to the arm
        self.active = active


class ResolverTransceiver(CANTransceiver):
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
        # Keys must match the joint names in the arm model
        self.joint_map =  {
            "base-rotation":    Joint("base-rotation", 0x04, True),
            "shoulder":         Joint("shoulder",      0x08, True),
            "elbow":            Joint("elbow",         0x0C, False),
            "j4":               Joint("j4",            0x10, False),
            "j5":               Joint("j5",            0x14, False),
            "j6":               Joint("j6",            0x18, False, gear_ratio=4),
            "spmx":             Joint("spmx",          0x20, True),
            "spmy":             Joint("spmy",          0x24, True),
            "spmz":             Joint("spmz",          0x28, True),
            "end-rotation":     Joint("end-rotation",  0x1C, False)
        }

        # Define an additonal transmitter for zeroing
        # Change the ID for sending to 0x0A3, make the receive timeout longer
        kwargs["arbitration_id"] = 0x0A3
        kwargs["receive_timeout"] = 0.5
        self.zero_transceiver = CANTransceiver(**kwargs)
        self.zero_transceiver.set_log_level("critical")
        self.zero_transceiver.logger = logger

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

    def reset_sector_count(self, joint_name: str) -> bool:
        """
        Method to reset the sector count for a given resolver

        Return True on success, False otherwise

        Assume the joint is within half a resolver-revolution of the zero point
        """
        joint = self.get_joint(joint_name)
        if joint.gear_ratio != 1:
            self.logger.info(f'Resetting sector for geared joint {joint_name}')
            
            # Get an initial reading
            integer_data = self.poll_resolver(joint_name)
            if integer_data is None:
                return False
            angle_data = self._convert_to_rad(integer_data)
            joint.last_reading = angle_data

            # Set sector
            # Joint angle will be close to zero, but may be just above or just below
            # Set sector to first or last depending on which one
            if angle_data < pi:
                # Joint is in the first sector
                joint.sector_count = 0
            if angle_data >= pi:
                # Joint is in the last sector
                joint.sector_count = joint.gear_ratio - 1

        return True
    
    def zero(self, joint_name: str) -> bool:
        """
        Method to zero a given resolver

        Returns True on success, False otherwise

        Raises KeyError if invalid joint name given
        """
        self.logger.info(f'Zeroing joint {joint_name}')
        # Send the zeroing command
        integer_data = self.poll_resolver(joint_name, self.zero_transceiver)
        return integer_data is not None
    
    def poll_resolver(self, joint_name: str, transceiver: CANTransceiver=None) -> int:
        """
        Method to poll a resolver and validate the output

        Returns the integer value from the resolver (if valid) or None (if invalid)

        Raises KeyError if invalid joint name given
        """
        resolver_id = self.get_joint(joint_name).id
        
        # Default to this transceiver
        if transceiver is None:
            transceiver = self
        
        # Pack and transmit binary data
        data = transceiver.pack([resolver_id])
        if not transceiver.transmit(data):
            transceiver.logger.error(f'Transmit timeout for joint {joint_name}')
            return None
        
        # Receive four bytes from the BASE board
        # The first is the resolver ID, the second are error flags,
        # The third and fourth are a single 14-bit value
        can_msg = transceiver.receive()
        if can_msg is None:
            transceiver.logger.error(f'CAN read timeout for joint {joint_name}')
            return None
        if len(can_msg.data) != calcsize(transceiver.receive_fmt):
            transceiver.logger.warn(f'Got a message of the wrong length for joint {joint_name}')
            return None
        received_id, flags, integer_data = transceiver.unpack(can_msg.data)
        # Verify the returned message
        if received_id != resolver_id:
            transceiver.logger.warn(f'Got the wrong resolver reply. Wanted {resolver_id}, got {received_id}')
            # Receive again so we eventually flush the receive buffer
            # Needed in case we don't have the most recent messages
            transceiver.receive()
            return None
        if flags & 0x01:
            transceiver.logger.warn(f'RS485 read timeout for joint {joint_name}')
            return None
        if flags & 0x02:
            transceiver.logger.warn(f'Invalid checksum from joint {joint_name}')
            return None
        
        # Return the integer message
        return integer_data
    
    def position(self, joint_name: str) -> float:
        """
        Method to read a given encoder

        Returns float value in [0, 2 pi) or None on failure

        Raises KeyError if invalid joint name given
        """
        # Poll resolver and decode into radians
        integer_data = self.poll_resolver(joint_name)
        if integer_data is None:
            return None
        angle_data = self._convert_to_rad(integer_data)
        
        # Get the joint object for post-processing
        joint = self.get_joint(joint_name)

        # Handle rotation counting if gear ratio is not 1
        if joint.gear_ratio != 1:
            # Update the sector count
            # Compare current and previous reading to determine if, and in which direction
            # we have crossed between 2*pi and 0
            # (3*pi)/4 chosen as an arbitrarily large angle to show that the resolver
            # must have crossed between 2*pi and 0
            if angle_data - joint.last_reading < -3*pi/4:
                # Resolver has crossed from 2*pi to 0, so joint is in the next sector
                joint.sector_count = (joint.sector_count + 1) % joint.gear_ratio
            elif angle_data - joint.last_reading > 3*pi/4:
                # Resolver has crossed from 0 to 2*pi, so joint is in the previous sector
                joint.sector_count = (joint.sector_count - 1) % joint.gear_ratio
            # Update last reading 
            joint.last_reading = angle_data

            # Modify the joint output angle to account for the gearing
            angle_data = (angle_data + 2*pi*joint.sector_count) / joint.gear_ratio

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
        super().__init__('resolver_publisher')

        # If starting the script by itself, don't wait for arm data
        # If starting from the arm launch file, override parameter as True
        use_arm_data = self.declare_parameter("use_arm_data", False).value

        if use_arm_data:
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
        else:
            # Start the node
            self.start_node()

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
        # Delay between each bus reading. In practice maxs out at 750+-50 us
        self.receive_deadtime = 0.0005
        # Time to wait for a valid reading
        self.receive_timeout = 0.01
        # Delay between each ROS publish. In practice maxs out at 15+-1 ms
        resolver_pub_timer_period = 0.01

        # Initialise the transceiver
        self.resolver_transceiver = ResolverTransceiver(
            logger = self.get_logger(),
            channel = 'can1',
            bitrate = 200000,
            filter_ids = [0x0A0],
            receive_timeout = self.receive_timeout,
            receive_fmt = '>BBH',  # Big-endian. uint8, uint8, uint16
            arbitration_id = 0x0A1,
            transmit_fmt = '>B',  # Big-endian. uint8
            )

        # Handle if the node is being run without the arm model
        use_arm_data = self.get_parameter("use_arm_data").value
        if not use_arm_data:
            # Set up the data structure that we would otherwise get from the arm nodes
            self.arm_config_info = ArmConfigInfo.Response()
            # Include all resolvers
            for joint_name in self.resolver_transceiver.joint_map.keys():
                self.arm_config_info.joint_names.append(joint_name)
            # Assume no joint limits
            num_joints = len(self.arm_config_info.joint_names)
            self.arm_config_info.joint_limits_lower = [-2*pi] * num_joints
            self.arm_config_info.joint_limits_upper = [2*pi] * num_joints

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

            # Initialisation if this resolver is geared
            if joint.gear_ratio != 1:
                self.logger.info(f'Getting initial reading for geared resolver {joint_name}')
                while not self.reset_sector_count(joint_name):
                    # If reset failed, wait and try again
                    time.sleep(self.receive_timeout)
        
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
            # Zero the joint
            response.success = self.resolver_transceiver.zero(joint_name)
            if response.success:
                response.message = f"Successfully transmitted zeroing data for joint {joint_name}"
            else:
                response.message = f"Failed to zero joint {joint_name}"
                return response

            # If successful, reset the sector if the joint is geared
            joint = self.resolver_transceiver.get_joint(joint_name)
            if joint.gear_ratio != 1:
                response.success = self.resolver_transceiver.reset_sector_count(joint_name)
                if response.success:
                    response.message += f"\nSuccessfully reset sector count for joint {joint_name}"
                else:
                    response.message += f"\nFailed to reset sector count got joint {joint_name}"
                    return response
            
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

            joint_position = self.resolver_transceiver.position(joint_name)
            if joint_position is not None:
                # Successful transmit and receive, update value to be published
                self.resolver_state.position[i] = joint_position
            # Delay a little to not overwhelm the RS485 bus
            time.sleep(self.receive_deadtime)

        self.resolver_state.header.stamp = self.get_clock().now().to_msg()
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
