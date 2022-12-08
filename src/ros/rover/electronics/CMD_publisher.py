#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This node receives data from the CMDs, such as
angular velocity, current, temperature and duty_cycle, 
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
EDITED:		10/10/2022
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
from collections import namedtuple
from typing import List
from collections import deque
import json

# Are we currently tuning PID constants? If so, poll at a higher frequency, 
# and publish every value as soon as it's read without taking means.
TUNING_PID = False

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
NUM_SCIENCE_MOTORS = 2

# The CMD CAN arbitration IDs
WHEEL_IDS = [0x410, 0x420, 0x430, 0x440, 0x450, 0x460]
WHEEL_PIVOT_IDS = [None] * NUM_WHEELS
# TODO: Input true CMD IDs 
ARM_MOTOR_IDS = [0x410, 0x420, 0x430, 0x440, 0x450, 0x460, 0x470]
SCIENCE_MOTOR_IDS = [0x480, 0x490]

# For storing queues of all different data 
QUEUE_LENGTH = 10  # If not TUNING_PIDS
CMDFeedbackQueue = namedtuple('CMDFeedbackQueue', ['currents', 'duty_cycles', 'v_angulars', 'v_linears', 'temps', 'intervals'])
# Hooooooo boy. 
CMDQueues = {
    TYPE_MOTOR: deque([[CMDFeedback() for _ in range(NUM_TYPE_MOTOR)]], maxlen=QUEUE_LENGTH)
        for TYPE_MOTOR, NUM_TYPE_MOTOR in zip(['wheel', 'pivot', 'arm', 'sci'], 
            [NUM_WHEELS, NUM_WHEELS if PIVOT_STEERING else 0, NUM_ARM_MOTORS, NUM_SCIENCE_MOTORS])
}

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
        if not TUNING_PID:
            self.pub_timer = self.create_timer(1.0/float(PUBLISH_RATE), self.publish_msg)

        # Log initialisation information
        self.get_logger().debug(f"Initialised the Wheel Publisher class with:"\
                                f"- {NUM_WHEELS} wheels\n"\
                                f"- {NUM_WHEELS} wheel pivots\n" if PIVOT_STEERING else "0 wheel pivots"\
                                f"- {NUM_ARM_MOTORS} arm CMDs\n"\
                                f"- {NUM_SCIENCE_MOTORS} science CMDs"
                                )

    # Method that looks for any changes in the data from the CAN lines
    def read_callback (self):
        # Loop through each CAN line and receive data
        # Tell the read_cans method what type of motor we are passing it, so it knows what values to fill out
        CMDQueues["wheel"].append(self.read_cans(self.wheel_cans, MotorType.WHEEL))
        CMDQueues["pivot"].append(self.read_cans(self.wheel_pivot_cans, MotorType.WHEEL_PIVOT))
        CMDQueues["arm"].append(self.read_cans(
            self.arm_cans[:6], MotorType.ARM_JOINT
            ).extend(
                self.read_cans(
                    self.arm_cans[-1], MotorType.ARM_EF
                )))
        CMDQueues["sci"].append(self.read_cans(self.science_cans, MotorType.SCIENCE))
        if TUNING_PID:
            self.publish_feedback()
            
    def read_cans(self, cans: List[CANReceiver], motor_type: MotorType):
        """Take a list of CANReceivers, read each of them into a CMDFeedback message, 
        then return the list of messages
        
        Arguments:
        cans -- list of can receivers of the same type
        motor_type -- type of motor - determines how/whether we calculate velocity
        Return: List of CANFeedback messages for all the CanReceivers we provided
        """
        ros_msgs = []
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
                    raw_omega, duty_cycle, current, interval = can_line.unpack(can_msg.data)
                    omega = self.convert_omega_to_SI(raw_omega)

                    ros_msg.duty_cycle = self.convert_duty_cycle(duty_cycle)
                    ros_msg.current = self.convert_current(current)
                    ros_msg.interval = self.convert_interval(interval)

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

                    # Get message bus and ID
                    ros_msg.bus = bus   # science and arm messages are on can1, otherwise can0
                    ros_msg.id = (can_msg.arbitration_id >> 4) & 0x3f

                    ros_msgs.append(ros_msg)

        return ros_msgs

    def average_messages(self, queue: deque) -> List[CMDFeedback]:
        """
        Take a queue of Lists of CMDFeedback, and average the CMDFeedback at corresponding positions
        in each list, then return a list of the averaged messages
        """
        averages = []
        queue_len = len(queue)

        # loop over each CMD 
        for i in range(len(queue[0])):
            # to store the average values
            avg_feedback = CMDFeedback()
            avg_feedback.bus = queue[0][i].bus
            avg_feedback.id = queue[0][i].id

            # calculate averages
            avg_feedback.omega = sum(cmds[i].omega for cmds in queue) / queue_len
            avg_feedback.vel = sum(cmds[i].vel for cmds in queue) / queue_len
            avg_feedback.duty_cycle = sum(cmds[i].duty_cycle for cmds in queue) / queue_len
            avg_feedback.current = sum(cmds[i].current for cmds in queue) / queue_len
            avg_feedback.interval = sum(cmds[i].interval for cmds in queue) / queue_len

            averages.append(avg_feedback)

        return averages

    # Publishes only the most recent message data
    def compile_feedback (self) -> CMDsFeedback:
        message = CMDsFeedback()
        message.header = Header()
        message.header.frame_id = "cmd_publisher"
        message.header.stamp = self.get_clock().now()

        if TUNING_PID:
            # take only the latest value
            message.wheels = CMDQueues["wheel"][-1]
            message.wheel_pivots = CMDQueues["pivot"][-1]
            message.arm_motors = CMDQueues["arm"][-1]
            message.science_motors = CMDQueues["sci"][-1]
        else:
            # average the queue of values
            message.wheels = self.average_messages(CMDQueues["wheel"])
            message.wheel_pivots = self.average_messages(CMDQueues["pivot"])
            message.arm_motors = self.average_messages(CMDQueues["arm"])
            message.science_motors = self.average_messages(CMDQueues["sci"])

        return message

    # Publishes the average of the last data readings exists
    def publish_feedback (self):
        message = self.compile_feedback()
        self.publisher.publish(message)
    
    # Converts a raw velocity to an RPM
    def convert_omega_to_SI (self, value: int) -> float:
        return value / MAX_INT16 * ENCODER_TO_RAD_PER_SEC
        
    # Converts the value of the duty_cycle to something sensible
    # Converts a signed integer into a float
    def convert_duty_cycle (self, value: int) -> float:
        return abs(value) / MAX_INT16 * 26.0

    # Converts interval to a realistic value
    def convert_interval(self, value: int) -> float:
        return value / MAX_INT16

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
