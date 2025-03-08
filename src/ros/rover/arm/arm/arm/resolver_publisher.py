#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 publisher code for the motor resolvers
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: resolver_publisher
TOPICS:
  - /arm/resolvers                       [sensor_msgs/JointState]    [Published]
SERVICES:
  - /arm/arm_config_info                     [core/ArmConfigInfo]        [Client]
  - /arm/resolver_zero_service               [core/StringTrigger]        [Server]
  - /arm/resolver_sector_zero_service        [core/StringTrigger]        [Server]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:     electronics
AUTHOR(S):   Jory Braun, Tom Newton, Josh Cherubino
EDITED BY:   Rohit Pilakkat, Jonathan Chin
CREATION:    14/02/2022
EDITED:      08/03/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
    - Setup appropriate QoS profile for publisher
    - Transition to JCAN once it is stable
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
from rclpy.impl.rcutils_logger import RcutilsLogger
from sensor_msgs.msg import JointState
from arm_interfaces.srv import StringTrigger
from arm_interfaces.srv import ArmConfigInfo

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

        # Store joint states in a dictionary when messages are received
        self.joint_states = {}

        # Create mapping of joint names to their respective Joint objects
        # Initialise using default discontinuity angles and active status,
        # update in the managing ROS node using info from the arm model
        # Keys must match the joint names in the arm model
        self.joint_map =  {
            "arm_j1":    
                Joint("arm_j1", 0x04, True),
            "arm_j2":    
                Joint("arm_j2", 0x08, True),
            "arm_j3":    
                Joint("arm_j3", 0x0C, False),
            "arm_j4":    
                Joint("arm_j4", 0x10, False),
            "arm_j5":    
                Joint("arm_j5", 0x14, False),
            "arm_j6":    
                Joint("arm_j6", 0x18, False, gear_ratio=4)
            # "spmx": 
            #     Joint("spmx", 0x20, True),
            # "spmy":      
            #     Joint("spmy", 0x24, True),
            # "spmz":      
            #     Joint("spmz", 0x28, True),
            # "end-rotation": 
            #     Joint("end-rotation", 0x1C, False)
        }

        # Define an additonal transmitter for zeroing
        # Change the ID for sending to 0x0A3, make the receive timeout longer
        kwargs["arbitration_id"] = 0x0A3
        kwargs["receive_timeout"] = 0.5
        self.zero_transceiver = CANTransceiver(**kwargs)
        self.zero_transceiver.set_log_level("critical")
        self.zero_transceiver.logger = logger

    def enable_auto_mode(self):
        """ Sends a CAN message to enable automatic resolver updates. """
        self.logger.info("Enabling automatic resolver mode (sending 0x0A2)...")
        enable_message = self.pack([0x00])  # Pack message with default polling period
        if not self.transmit(enable_message):
            self.logger.error("Failed to send auto mode enable command!")

    def check_for_messages(self):
        """ Checks for new CAN messages and processes them. """
        can_msg = self.receive()
        if can_msg:
            self.process_incoming_message(can_msg)

    def process_incoming_message(self, can_msg):
        """ Processes incoming CAN messages and stores resolver readings. """
        if can_msg is None:
            return
    
        #Verify the CAN ID is 0x0A0
        if can_msg.arbitration_id != 0x0A0:
            return  # Skip any other IDs

        # Verify the data length is exactly 4 bytes
        expected_size = calcsize(self.receive_fmt)  # should be 4 if receive_fmt=">BBH"
        if len(can_msg.data) != expected_size:
            self.logger.warn(
                f"Ignoring 0x{can_msg.arbitration_id:X} frame: "
                f"expected {expected_size} bytes, got {len(can_msg.data)}."
            )
            return

        received_id, flags, integer_data = self.unpack(can_msg.data)

        # Identify which joint the message belongs to
        joint_name = None
        for name, joint in self.joint_map.items():
            if joint.id == received_id:
                joint_name = name
                break

        if joint_name is None:
            self.logger.warn(f"Received unknown resolver ID: {received_id}")
            return

        # Process data like poll_resolver did
        if flags & 0x01:
            self.logger.warn(f"RS485 read timeout for {joint_name}")
            return
        if flags & 0x02:
            self.logger.warn(f"Invalid checksum from {joint_name}")
            return

        # Store the integer data for use in position()
        self.joint_states[joint_name] = integer_data

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

            # Instead of poll_resolver, just read from the auto-updated joint_states
            integer_data = self.joint_states.get(joint_name, None)
            if integer_data is None:
                self.logger.warn(f"No auto-stream data for {joint_name}, cannot reset sector.")
                return False

            angle_data = self._convert_to_rad(integer_data)
            joint.last_reading = angle_data

            # Decide which sector to put the joint in:
            #   - If angle < π, assume it is near 0 sector
            #   - If angle >= π, assume it is near the last sector
            if angle_data < pi:
                joint.sector_count = 0
            else:
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
        Returns the latest received resolver position for the given joint.
        """
        # Get the stored integer value
        integer_data = self.joint_states.get(joint_name, None)
        if integer_data is None:
            return None  # No data received yet

        # Convert to radians
        angle_data = self._convert_to_rad(integer_data)

        # Process based on joint properties
        joint = self.get_joint(joint_name)

        # Handle gear ratio-based rotation counting
        if joint.gear_ratio != 1:
            if joint.last_reading is None:
                self.logger.info(f'Initializing reading for {joint_name}')
                success = self.reset_sector_count(joint_name)
                if not success:
                    return None
            elif angle_data - joint.last_reading < -3 * pi / 4:
                joint.sector_count = (joint.sector_count + 1) % joint.gear_ratio
            elif angle_data - joint.last_reading > 3 * pi / 4:
                joint.sector_count = (joint.sector_count - 1) % joint.gear_ratio

            joint.last_reading = angle_data
            angle_data = (angle_data + 2 * pi * joint.sector_count) / joint.gear_ratio

        # Reverse direction if necessary
        if joint.reverse:
            angle_data = self._reverse_direction(angle_data)

        # Move discontinuity out of normal joint motion range
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
        Start the node and make a service request to /arm/arm_config_info
        """
        super().__init__('resolver_publisher')

        # If starting the script by itself, don't wait for arm data
        # If starting from the arm launch file, override parameter as True
        use_arm_data = self.declare_parameter("use_arm_data", False).value

        if use_arm_data:
            # Create the client for /arm/arm_config_info
            self.client = self.create_client(ArmConfigInfo, "/arm/arm_config_info")
            # Wait for the service to become available
            while not self.client.wait_for_service(timeout_sec=1.0):
                self.get_logger().info("Service /arm/arm_config_info not available, waiting again...")
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
        Check if /arm/arm_config_info has responded. If so, save the data and set up the node
        """
        if self.future.done():
            # Got a response!
            self.get_logger().info("Got a response from /arm/arm_config_info. Starting the node.")
            self.client_check_timer.cancel()
            self.arm_config_info = self.future.result()
            self.start_node()
        else:
            self.get_logger().info("Failed to get response from /arm/arm_config_info, waiting again...")

    def start_node(self):
        """
        Setup the node for the application. Create publishers, services, and initialize data members.
        """
        # Delay between each bus reading. In practice maxs out at 750+-50 us
        self.receive_deadtime = 0.001
        # Time to wait for a valid reading
        self.receive_timeout = 0.1
        # Delay between each ROS publish. In practice maxs out at 15+-1 ms
        resolver_pub_timer_period = 0.015

        # Initialize the transceiver (keep it as originally structured)
        self.resolver_transceiver = ResolverTransceiver(
            logger=self.get_logger(),
            channel="can1",
            bitrate=200000,
            filter_ids=[0x0A0],
            receive_timeout=self.receive_timeout,
            receive_fmt=">BBH",  # Big-endian. uint8, uint8, uint16
            arbitration_id=0x0A2,
            transmit_fmt=">B",  # Big-endian. uint8
        )

        # Immediately enable auto mode once at startup
        self.resolver_transceiver.enable_auto_mode()

        # Create a 1 Hz timer to keep re-sending 0x0A2
        self.enable_auto_timer = self.create_timer(1.0, self.enable_auto_timer_callback)


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
            self.arm_config_info.joint_limits_lower = [-2 * pi] * num_joints
            self.arm_config_info.joint_limits_upper = [2 * pi] * num_joints

        # Create the output message type to track the resolver state
        self.resolver_state = JointState()
        joint_names = self.arm_config_info.joint_names
        self.resolver_state.name = joint_names
        self.resolver_state.position = [0.0] * len(joint_names)
        # Initialize unused fields to have correct lengths for consistency
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
        self.joint_states_publisher = self.create_publisher(JointState, "/arm/joint_states", 10)

        # Timer to periodically check for new CAN messages
        self.create_timer(0.01, self.process_can_messages)

        # Timer to publish joint states
        self.create_timer(resolver_pub_timer_period, self.publish)

        # Construct the service to zero resolvers
        self.zero_service = self.create_service(StringTrigger, "/arm/resolver_zero_service", self.zero_callback)
        # Construct the service to zero resolver sector
        self.sector_zero_service = self.create_service(StringTrigger, "/arm/resolver_sector_zero_service", self.sector_zero_callback)

    def enable_auto_timer_callback(self):
        """Re-send the 0x0A2 command every 1 second."""
        self.get_logger().info("Re-sending 0x0A2 enable auto mode command...")y
        self.resolver_transceiver.enable_auto_mode()

    def process_can_messages(self):
        while True:
            can_msg = self.resolver_transceiver.receive()
            if can_msg is None:
                break
            self.resolver_transceiver.process_incoming_message(can_msg)

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

            # -- Re-enable auto mode immediately after zeroing --
            if response.success:
                self.get_logger().info("Re-enabling auto mode after zeroing resolvers.")
                self.resolver_transceiver.enable_auto_mode()
            
        except KeyError as e:
            response.success = False
            response.message = str(e).replace("'", "")
        return response
    
    def sector_zero_callback(self, request: StringTrigger.Request, response: StringTrigger.Response):
        """
        Callback for the resolver sector zero service
        """
        joint_name = request.value
        try:
            # Reset the sector if the joint is geared
            joint = self.resolver_transceiver.get_joint(joint_name)
            if joint.gear_ratio != 1:
                response.success = self.resolver_transceiver.reset_sector_count(joint_name)
                if response.success:
                    response.message = f"\nSuccessfully reset sector count for joint {joint_name}"
                else:
                    response.message = f"\nFailed to reset sector count got joint {joint_name}"
            else:
                response.success = True
                response.message = f"\nJoint {joint_name} is not a geared joint"

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
        """Publishes the latest joint states every 10ms."""
        resolver_state = JointState()
        resolver_state.name = list(self.resolver_transceiver.joint_map.keys())

        resolver_state.position = [
            self.resolver_transceiver.position(joint_name) or 0.0
            for joint_name in resolver_state.name
        ]

        resolver_state.header.stamp = self.get_clock().now().to_msg()
        self.joint_states_publisher.publish(resolver_state)

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
