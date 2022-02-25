#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This node receives data from the wheels, such as
velocity, current and power, and is able to publish
over ROS. It uses the CAN receiver class to read the
data published over the network.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: wheel_publisher
TOPICS:
  - /electronics/wheel_data  [WheelData]   [Published]
  - /control/drive_inputs    [DriveInput]  [Subscribed]
  - /autonomous/drive_inputs [DriveInput]  [Subscribed]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Harrison Verrios
CREATION:	18/02/2022
EDITED:		18/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include ROS packages
import rclpy, time, datetime
from rclpy.node import Node
import time

# Import the wheel message type
from core.msg import WheelData, DriveInput

# Import the CAN libary
from coms_utils.can_interface import CANReceiver

# The Wheel CAN arbitration IDs
WHEEL_IDS = [0x410, 0x420, 0x430, 0x440, 0x450, 0x460]

# Mathematical PI
PI = 3.141593


'''
The following are the adjustable parameters that can
be configured for the wheels.
'''

# The number of recent points to take the average from
# Use '1' for no average system
STORED_DATA_LEN = 10

NUM_WHEELS = 6

# The value that 1.0 velocity maps to in RPM
# Calculated using a Tacometer
ENCODER_TO_RPM = 92.9

# Store the wheel radius [m]
WHEEL_RADIUS = 0.122



# Main Wheel Publisher class
class WheelPublisher (Node):

    # Stores the current message values
    message: WheelData = WheelData()
    
    # Stores the properties as an array of arrays
    rpms = []
    powers = []
    
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

        # Set up the CAN interface for each wheel id
        self.cans = []
    
        # Create the CAN network
        self.cans = [CANReceiver(channel="can0", filter_ids=[WHEEL_IDS[i]], receive_timeout=0.1, receive_fmt="<hh", display=False) for i in range(NUM_WHEELS)]
        
        # Set up the average arrays
        self.rpms.append([0 for i in range(STORED_DATA_LEN)])
        self.powers.append([0 for i in range(STORED_DATA_LEN)])
        

        # Create the publisher
        self.publisher = self.create_publisher(WheelData, "/electronics/wheel_data", 10)

        # Create a subscriber to drive commands
        self.subscription_m = self.create_subscription(DriveInput, "/control/drive_inputs", self.drive_callback, 10)
        self.subscription_a = self.create_subscription(DriveInput, "/autonomous/drive_inputs",  self.drive_callback, 10)

        # Create a time to constantly loop and check for data
        self.read_timer = self.create_timer(0.01, self.read_callback)

        # Create a timer to publish the current data
        self.pub_timer = self.create_timer(0.1, self.publish_msg)

        # Create a timer to clear the current message data if nothing has happened
        self.clear_timer = self.create_timer(0.1, self.clear_msg)

    
    # Method that looks for any changes in the data from the CAN lines
    def read_callback (self):
        # Loop through each CAN line and receive data
        for i in range(6):
            can_msg = self.cans[i].receive()
            print("got can msg")
            # If a message exists
            if can_msg:
                # Read the velocity data
                rpm = can_msg.data[:2]
                rpm = int.from_bytes(rpm, "little", signed=True)
                # Get a negative for some wheels
                if i <= 2: rpm *= -1
                self.rpms[i].append(self.convert_rpm(rpm))
                del self.rpms[i][0]
                
                # Read the power data
                power = can_msg.data[2:]
                power = int.from_bytes(power, "little", signed=True)               
                self.powers[i].append(self.convert_power(power))
                del self.powers[i][0]
                
                # Update the timestamp
                self.time = time.time() 

    # Callback that reads an input message from the drive commands
    # Outputs are only valid when a drive message comes through
    def drive_callback (self, msg):
        if abs(msg.speed) > 0.0 or abs(msg.steer) > 0.0:
            self.valid = True
        else:
            self.valid = False  


    # Publishes the current message data that exists
    def publish_msg (self):
    
        # Get the average data in the message
        for i in range(6):
            self.message.rpms[i]  = sum(self.rpms[i]) / float(STORED_DATA_LEN)
            self.message.powers[i]      = sum(self.powers[i]) / float(STORED_DATA_LEN)
            self.message.velocities[i]  = self.convert_rpm_to_vel (self.message.rpms[i])

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
        return abs(value) / 32768.0


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
