#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This node receives data from the wheels, such as
velocity, current and power, and is able to publish
over ROS. It uses the CAN receiver class to read the
data published over the network.

This program operates by polling the can buss for encoder values and adding them to a queue with max len 10,
then publishing the average value of the data in the queue to ROS. It only publishes non zero values if inputs
are being sent to the wheels - otherwise it resets the queue to empty.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: wheel_publisher
TOPICS:
  - /electronics/wheel_data  [WheelData]   [Published]
  - /control/drive_inputs    [DriveInput]  [Subscribed]
  - /autonomous/drive_inputs [DriveInput]  [Subscribed]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Harrison Verrios, Liam Whittle
CREATION:	18/02/2022
EDITED:		07/06/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include ROS packages
import rclpy
from rclpy.node import Node
import time

# Import the wheel message type
from core.msg import WheelData, DriveInput

# Import the CAN library
from coms_utils.can_interface import CANReceiver

# Import QoS profile
from rclpy.qos import qos_profile_sensor_data as qos

# The Wheel CAN arbitration IDs
WHEEL_IDS = [0x410, 0x420, 0x430, 0x440, 0x450, 0x460]

# Mathematical PI
PI = 3.1415926535897932384

'''
The following are the adjustable parameters that can
be configured for the wheels.
'''

NUM_WHEELS = 6

# The value that 1.0 velocity maps to in RPM
# Calculated using a Tacometer
ENCODER_TO_RPM = 92.9

# Store the wheel radius [m]
WHEEL_RADIUS = 0.122

# The rate (times per second) to poll the CAN bus and publish the wheels at
POLL_RATE = 100
PUBLISH_RATE = 20


# Main Wheel Publisher class
class WheelPublisher (Node):

    # Stores the current message values
    message: WheelData = WheelData()
    
    # Whether to ignore data
    valid: bool = False

    # Constructor sets up the publisher
    def __init__ (self):
    
        # Set up the node
        super().__init__("wheel_publisher")

        # Print initialisation information
        print("Initialising the Wheel Publisher class.")

        # Store the starting time
        self.t = time.time()
    
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
        self.publisher = self.create_publisher(WheelData, "/electronics/wheel_data", 10)

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

                    # RPM operating as a FIFO Queue
                    self.rpms[i] = self.convert_rpm(rpm)
                    
                    # Power operating as a FIFO Queue
                    self.powers[i] = self.convert_power(power)

                    # Current operating as a FIFO Queue
                    self.currents[i] = self.convert_current(current)
                    
                    # Update the timestamp
                    self.t = time.time()
            
            # In case of an eror, just skip and continue
            except:
                continue


    # Callback that reads an input message from the drive commands
    # Outputs are only valid when a drive message comes through
    def drive_callback(self, msg):
        self.valid = abs(msg.speed) > 0.0 or abs(msg.steer) > 0.0

        # if we aren't driving, we shouldn't accept any previous values in our average
        if not self.valid:
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
            self.message.currents[i] = self.currents[i]

        # Check for invalid data, reset the message
        if not self.valid:
            self.message = WheelData()

        # Publish the data
        self.publisher.publish(self.message)

    
    # Clears the current message if nothing has happened in a while
    def clear_msg (self):
        
        # Check if the last message was a while ago
        if time.time() - self.t > 0.5:
            # Clear the message
            self.message = WheelData()
     
            
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
    publisher = WheelPublisher()
    rclpy.spin(publisher)

    # Clean up when complete
    publisher.destroy_node()
    rclpy.shutdown()


# Called when the script executes
if __name__=="__main__":
    main()
