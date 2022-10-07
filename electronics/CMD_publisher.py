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
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Harrison Verrios, Liam Whittle, Max Tory
CREATION:	18/02/2022
EDITED:		07/10/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include ROS packages
import rclpy
from rclpy.node import Node
import time

# Import the wheel message type
from core.msg import CMDFeedback, CMDsFeedback, DriveInput

# Import the CAN library
from coms_utils.can_interface import CANReceiver

# Import QoS profile
from rclpy.qos import qos_profile_sensor_data as qos

# Mathematical PI
PI = 3.1415926535897932384

# The value that 1.0 velocity maps to in RPM
# Calculated using a Tacometer
ENCODER_TO_RPM = 92.9

# Store the wheel radius [m]
WHEEL_RADIUS = 0.122

# ROS timing constants
POLL_RATE = 100
PUBLISH_RATE = 20

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
SCIENCE_MOTOR_IDS = [] * NUM_SCIENCE_MOTORS


# Main CMD Publisher class
class CMDPublisher (Node):
    # Stores the current message values
    message: CMDsFeedback = CMDsFeedback()
    ignore_data: bool = True

    # Constructor sets up the publisher
    def __init__ (self):
    
        # Set up the node
        super().__init__("CMD_publisher")

        # Log initialisation information
        self.get_logger().debug("Initialising the Wheel Publisher class.")

        # Store the starting time
        self.last_read = time.time()
    
        # Create the CAN network
        self.cans = [CANReceiver(channel="can0", filter_ids=[WHEEL_IDS[i]], # can channel and ids of wheels
                                 receive_timeout=1, # seconds to wait for message
                                 receive_fmt="<hhh", # 3 shorts in little-endian format 
                                 bitrate=200000)
                     for i in range(NUM_WHEELS)]
        self.rpms = [0 for _ in range(NUM_WHEELS)]
        self.powers = [0 for _ in range(NUM_WHEELS)]
        self.currents = [0 for _ in range(NUM_WHEELS)]

        # Create the publisher
        self.publisher = self.create_publisher(CMDsFeedback, "/electronics/cmd_feedback", 10)

        # Create a subscriber to drive commands
        self.subscription_man = self.create_subscription(DriveInput, "/control/drive_inputs", self.drive_callback, qos)
        self.subscription_auto = self.create_subscription(DriveInput, "/autonomous/drive_inputs",  self.drive_callback, 10)

        # Create a time to constantly loop and check for data
        self.read_timer = self.create_timer(1.0/float(POLL_RATE), self.read_callback)

        # Create a timer to publish the current data
        self.pub_timer = self.create_timer(1.0/float(PUBLISH_RATE), self.publish_msg)


    # Method that looks for any changes in the data from the CAN lines
    def read_callback (self):
        # Loop through each CAN line and receive data

        for i, can_line in enumerate(self.cans):
            # Catch for an error that come about with the wheels
            try:
                can_msg = can_line.receive()
                
                # If a message exists
                if can_msg:
                    # Read the velocity data
                    rpm, power, current = can_line.unpack(can_msg.data)
                    # Get a negative for wheels on one side due to motor orientation 
                    if i <= 2: rpm *= -1

                    # Most recent angular vel reading 
                    self.rpms[i] = self.convert_rpm(rpm)
                    
                    # Most recent power reading
                    self.powers[i] = self.convert_power(power)

                    # Most recent current reading
                    self.currents[i] = self.convert_current(current)
                    
                    # Update the timestamp
                    self.last_read = time.time()
            
            # In case of an eror, just skip and continue
            except:
                continue


    # Callback that reads an input message from the drive commands
    # Outputs are only valid when a drive message comes through
    def drive_callback(self, msg):
        self.ignore_data = msg.speed == 0.0 and msg.steer == 0.0

        # if we aren't driving, we shouldn't accept any previous values in our average
        if self.ignore_data:
            # Set up the average arrays
            self.rpms = [0 for _ in range(NUM_WHEELS)]
            self.powers = [0 for _ in range(NUM_WHEELS)]
            self.currents = [0 for _ in range(NUM_WHEELS)]

    # Publishes the current message data that exists
    def publish_msg (self):
        # Get the average data in the message
        for i in range(NUM_WHEELS):
            self.message.rpms[i] = self.rpms[i]
            self.message.powers[i] = self.powers[i]
            self.message.velocities[i] = self.convert_rpm_to_vel(self.message.rpms[i])

        # Check for invalid data, reset the message
        if self.ignore_data:
            self.message = CMDsFeedback()

        # Publish the data
        self.publisher.publish(self.message)

    
    # Clears the current message if nothing has happened in a while
    def clear_msg (self):
        
        # Check if the last message was a while ago
        if time.time() - self.last_read > 0.5:
            # Clear the message
            self.message = CMDsFeedback()
     
            
    # Converts a raw velocity to an RPM
    def convert_rpm (self, value: int) -> float:
        return value / 32768.0 * ENCODER_TO_RPM
        
    # Converts the value of the power to something sensible
    # Converts a signed integer into a float
    def convert_power (self, value: int) -> float:
        return abs(value) / 32768.0 * 26.0

    # Converts the value of the current to something sensible
    # Converts a signed integer into a float
    def convert_current (self, value: int) -> float:
        return float(abs(value)) / 4096.0 * 2.5 * 10.0

    # Converts the RPM value to a speed in m/s
    def convert_rpm_to_vel (self, rpm: float) -> float:
        return rpm / 60.0 * 2 * PI * WHEEL_RADIUS
    

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
