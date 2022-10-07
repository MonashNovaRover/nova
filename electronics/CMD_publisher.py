#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This node receives data from the CMDs, such as
angular velocity, current, temperature and power, 
and is able to publish over ROS. It uses the CAN 
receiver class to read the data published over the 
network.

This program operates by polling the can bus for 
encoder values and adding them to a queue with max 
len 10, then publishing the average value of the 
data in the queue to ROS. It only publishes non-zero 
values if inputs are being sent to the wheels - 
otherwise it resets the queue to empty.

Modified from the initial wheel_publisher.py script
by Harrison Verrios and Liam Whittle to be
generalised to all CMDs by Max Tory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: wheel_publisher
TOPICS:
  - /electronics/cmd_feedback  [CMDsFeedback]   [Published]
  - /control/drive_inputs    [DriveInput]  [Subscribed]
  - /autonomous/drive_inputs [DriveInput]  [Subscribed]
TODO:
  - Get Arm CMD Can IDs
  - Decide on whether to average past CMD vals
  - Classes for different CMD collections (Wheels,
    Wheel pivots, Arm motors, etc to eliminate
    god class)
  - Get temperature data from CMDs?
  - Ensure that timestamps etc are correct even if
    we are not sending wheel spins
  - Still send other motors when wheels are not spinning
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Harrison Verrios, Liam Whittle, Max Tory
CREATION:	18/02/2022
EDITED:		07/10/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include ROS packages
from matplotlib.pyplot import sci
import rclpy
from rclpy.node import Node
import time
from std_msgs.msg import Header

# Import the wheel message type
from core.msg import CMDFeedback, CMDsFeedback, DriveInput

# Import the CAN library
from coms_utils.can_interface import CANReceiver

# Import QoS profile
from rclpy.qos import qos_profile_sensor_data as qos

# For defining different motor types
from enum import Enum
from typing import List

# Mathematical PI
PI = 3.1415926535897932384

# The value that 1.0 velocity maps to in radians per second
# Calculated using a Tachometer
ENCODER_TO_RAD_PER_SEC = 9.728465

# For converting angular to linear velocities (M)
WHEEL_RADIUS = 0.122
END_EFFECTOR_BOLT_PITCH = 1.5e-3

# Maximum value of a 16bit (signed) integer as float for float division
MAX_INT16 = 32768.

# ROS timing constants
POLL_RATE = 100
PUBLISH_RATE = 20

# Motor types
class MotorType(Enum):
    WHEEL: 0
    WHEEL_PIVOT: 1
    ARM_JOINT: 2
    ARM_EF: 3
    SCIENCE: 4
    

# The following are the adjustable parameters that can
# be configured for the CMDs.

# Motor definitions
NUM_WHEELS = 6
PIVOT_STEERING = False
NUM_ARM_MOTORS = 7
NUM_SCIENCE_MOTORS = 0

# The CMD CAN arbitration IDs
WHEEL_IDS = [0x410, 0x420, 0x430, 0x440, 0x450, 0x460]
WHEEL_PIVOT_IDS = [None] * NUM_WHEELS
# TODO: Input true CMD IDs 
ARM_MOTOR_IDS = [0x111] * NUM_ARM_MOTORS
SCIENCE_MOTOR_IDS = [0x470] * NUM_SCIENCE_MOTORS


# Main CMD Publisher class
class CMDPublisher (Node):
    # Stores the current message values
    message: CMDsFeedback = CMDsFeedback()
    ignore_data: bool = True

    # Constructor sets up the publisher
    def __init__ (self):
    
        # Set up the node
        super().__init__("CMD_publisher")

        # Store the starting time
        self.last_read = time.time()
    
        # Create the CAN network
        # receivers for the wheels on the rover
        self.wheel_cans = [CANReceiver(channel="can0", filter_ids=[WHEEL_IDS[i]], # can channel and ids of wheels
                                 receive_timeout=1, # seconds to wait for message
                                 receive_fmt="<hhh", # 3 shorts in little-endian format 
                                 bitrate=200000) # bitrate of can line
                     for i in range(NUM_WHEELS)]

        if PIVOT_STEERING:
            self.wheel_pivot_cans = [CANReceiver(channel="can0", filter_ids=[WHEEL_PIVOT_IDS[i]], # can channel and ids of wheels
                                    receive_timeout=1, # seconds to wait for message
                                    receive_fmt="<hhh", # 3 shorts in little-endian format 
                                    bitrate=200000) # bitrate of can line
                        for i in range(NUM_WHEELS)]

        self.arm_cans = [CANReceiver(channel="can1", filter_ids=[ARM_MOTOR_IDS[i]], # can channel and ids of wheels
                                 receive_timeout=1, # seconds to wait for message
                                 receive_fmt="<hhh", # 3 shorts in little-endian format 
                                 bitrate=200000) # bitrate of can line
                     for i in range(NUM_ARM_MOTORS)]

        self.science_cans = [CANReceiver(channel="can1", filter_ids=[SCIENCE_MOTOR_IDS[i]], # can channel and ids of wheels
                                 receive_timeout=1, # seconds to wait for message
                                 receive_fmt="<hhh", # 3 shorts in little-endian format 
                                 bitrate=200000) # bitrate of can line
                     for i in range(NUM_SCIENCE_MOTORS)]

        # Create the publisher
        self.publisher = self.create_publisher(CMDsFeedback, "/electronics/cmd_feedback", 10)

        # Create a subscriber to drive commands
        self.subscription_man = self.create_subscription(DriveInput, "/control/drive_inputs", self.drive_callback, qos)
        self.subscription_auto = self.create_subscription(DriveInput, "/autonomous/drive_inputs",  self.drive_callback, 10)

        # Create a time to constantly loop and check for data
        self.read_timer = self.create_timer(1.0/float(POLL_RATE), self.read_callback)

        # Create a timer to publish the current data
        self.pub_timer = self.create_timer(1.0/float(PUBLISH_RATE), self.publish_msg)

        # Log initialisation information
        self.get_logger().debug(f"Initialised the Wheel Publisher class with:"\
                                f"- {NUM_WHEELS} wheels\n"\
                                f"- {NUM_WHEELS} wheel pivots\n" if PIVOT_STEERING else ""\
                                f"- {NUM_ARM_MOTORS} arm CMDs\n"\
                                f"- {NUM_SCIENCE_MOTORS} science CMDs"
                                )

    def construct_message(self, wheel_data: List[CMDFeedback], wheel_pivot_data: List[CMDFeedback], arm_data: List[CMDFeedback], science_data: List[CMDFeedback]):
        """Constructs a CMDsFeedback data type given the following data

        Args:
            wheel_data (List[CMDFeedback]): wheel CMD feedback
            wheel_pivot_data (List[CMDFeedback]): wheel pivot CMD feedback
            arm_data (List[CMDFeedback]): arm motor CMD feedback
            science_data (List[CMDFeedback]): science motor CMD feedback
        """
        self.message = CMDsFeedback()
        # create header with frame ID and timestamp
        header = Header()
        header.frame_id = "cmd_feedback"
        header.stamp = self.get_clock().now()

        # Filling the message with the given data
        self.message.header = header
        self.message.wheels = wheel_data
        self.message.wheel_pivots = wheel_pivot_data
        self.message.arm_motors = arm_data
        self.message.science_motors = science_data

    # Method that looks for any changes in the data from the CAN lines
    def read_callback (self):
        # Loop through each CAN line and receive data
        wheel_data = self.read_cans(self.wheel_cans, MotorType.WHEEL)
        wheel_pivot_data = self.read_cans(self.wheel_pivot_cans, MotorType.WHEEL_PIVOT)
        arm_data = self.read_cans(
            self.arm_cans[:6], MotorType.ARM_JOINT
            ).extend(
                self.read_cans(
                    self.arm_cans[-1], MotorType.ARM_EF
                ))
        science_data = self.read_cans(self.science_cans, MotorType.SCIENCE)
        self.construct_message(wheel_data, wheel_pivot_data, arm_data, science_data)
            
    def read_cans(self, cans: CANReceiver, motor_type: MotorType):
        """Take a list of CANReceivers, read each of them into a CMDFeedback message, 
        then return the list of messages
        
        Arguments:
        cans -- list of can receivers of the same type
        motor_type -- type of motor - determines how/whether we calculate velocity
        Return: List of CANFeedback messages for all the CanReceivers we provided
        """
        ros_msgs = [None] * 6
        for i, can_line in enumerate(cans):
            # Catch for an error that come about with the wheels
            try:
                can_msg = can_line.receive()
                
            # In case of an eror, just skip and continue
            except Exception as e:
                self.get_logger().warn("Error reading CAN lines! Continuing...")
            # In case of no error, get data from the can msg
            else:
                # If a message exists
                if can_msg:
                    ros_msg = CMDFeedback()
                    # Read the can data
                    raw_omega, power, current = can_line.unpack(can_msg.data)
                    omega = self.convert_omega_to_SI(raw_omega)

                    ros_msg.power = self.convert_power(power)
                    ros_msg.current = self.conver_current(current)

                    if motor_type == MotorType.WHEEL:
                        # Get a negative for wheels on one side due to motor orientation 
                        if i <= 2: omega *= -1
                        # Wheels have a linear velocity based on their radius 
                        ros_msg.vel = self.convert_angular_to_linear_wheel(omega)
                        bus = 0
                    elif motor_type == MotorType.ARM_EF:
                        ros_msg.vel = self.convert_angular_to_linear_end_effector(omega)
                        bus = 1
                    elif motor_type == MotorType.ARM_JOINT or motor_type == MotorType.SCIENCE:
                        bus = 1
                    else:
                        bus = 0
                        
                    ros_msg.omega = omega
                    
                    # Update the timestamp
                    self.last_read = time.time()

                    # Get message bus and ID
                    ros_msg.bus = bus   # science and arm messages are on can1, otherwise can0
                    ros_msg.id = (can_msg.arbitration_id >> 4) & 0x3f
                    # set timestamp on message
                    ros_msg.time = int(self.last_read * 1000)

                    ros_msgs[i] == ros_msg


    # Callback that reads an input message from the drive commands
    # Outputs are only valid when a drive message comes through
    def drive_callback(self, msg):
        self.ignore_data = msg.speed == 0.0 and msg.steer == 0.0

    # Publishes the current message data that exists
    def publish_msg (self):
        if self.ignore_data:
            self.clear_msg()

        # Publish the data
        self.publisher.publish(self.message)

    
    # Clears the current message if nothing has happened in a while
    def clear_msg (self):
        self.message = CMDsFeedback()
     
    # Converts a raw velocity to an RPM
    def convert_omega_to_SI (self, value: int) -> float:
        return value / MAX_INT16 * ENCODER_TO_RAD_PER_SEC
        
    # Converts the value of the power to something sensible
    # Converts a signed integer into a float
    def convert_power (self, value: int) -> float:
        return abs(value) / MAX_INT16 * 26.0

    # Converts the value of the current to something sensible
    # Converts a signed integer into a float
    def convert_current (self, value: int) -> float:
        return float(abs(value)) / 4096.0 * 2.5 * 10.0

    # Converts the angular velocity value to a speed in m/s based on the wheel radius
    def convert_angular_to_linear_wheel (self, omega: float) -> float:
        return omega * WHEEL_RADIUS
    
    # Converts the angular velocity value to a speed in m/s based on the end effector bolt pitch
    def convert_angular_to_linear_end_effector (self, omega: float) -> float:
        return omega * END_EFFECTOR_BOLT_PITCH / (2 * PI)
    

# Main function sets up the ROS class
def main(args=None):

    # Create the publisher
    rclpy.init(args = args)
    publisher = CMDPublisher()

    # Listen and publish forever
    rclpy.spin(publisher)

    # Clean up when complete
    publisher.destroy_node()
    rclpy.shutdown()


# Called when the script executes
if __name__=="__main__":
    main()
