#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This node receives data from the wheels, such as
velocity, current and power, and is able to publish
over ROS. It uses the CAN receiver class to read the
data published over the network.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: radio_monitor
TOPICS:
  - /electronics/wheel_data       [WheelData]
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

# Import the wheel message type
from core.msg import WheelData

# Import the CAN libary
from coms_utils.can_interface import CANReceiver

# The Wheel CAN arbitration IDs
WHEEL_IDS = [0x410, 0x420, 0x430, 0x440, 0x450, 0x460]

# Maximum number of average
STORED_DATA_LEN = 10

# The value that 1.0 velocity maps to in RPM
ENCODER_TO_RPM = 92.9

# Store the wheel radius [m]
WHEEL_RADIUS = 0.122

# Mathematical PI
PI = 3.141593


# Main Wheel Publisher class
class WheelPublisher (Node):

    # Stores the current message values
    message: WheelData = WheelData()
    
    # Stores the velocities as an array of arrays
    velocities = []
    powers = []

    # Constructor sets up the publisher
    def __init__ (self):

        # Print initialisation information
        print("Initialising the Wheel Publisher class.")

        # Store the starting time
        self.time = datetime.datetime.now()

        # Set up the CAN interface for each wheel id
        self.cans = []
        for i in range(6):
            self.cans.append(CANReceiver(channel="can0", filter_ids=[WHEEL_IDS[i]], receive_timeout=0.1, receive_fmt="<hh"))
            self.velocities.append([0 for i in range(STORED_DATA_LEN)])
            self.powers.append([0 for i in range(STORED_DATA_LEN)])
        

        # Create the publisher
        super().__init__("wheel_publisher")
        self.publisher = self.create_publisher(WheelData, "/electronics/wheel_data", 10)

        # Create a time to constantly loop and check for data
        self.read_timer = self.create_timer(0.01, self.read_callback)

        # Create a timer to publish the current data
        self.pub_timer = self.create_timer(0.1, self.publish_msg)

        # Create a timer to clear the current message data if nothing has happened
        self.clear_timer = self.create_timer(0.1, self.clear_msg)

    
    # Method that looks for any changes in the data from the CAN lines
    def read_callback (self):
        # Loop through each can line and look
        for i in range(6):
            can_msg = self.cans[i].receive()

            # If a message exists
            if can_msg:
            
                # Read the velocity data
                vel = can_msg.data[:2]
                vel = int.from_bytes(vel, "little", signed=True)
                # Get a negative for some wheels
                if i <= 2: vel *= -1
                self.velocities[i].append(self.convert_vel(vel))
                del self.velocities[i][0]
                
                # Read the power data
                power = can_msg.data[2:]
                power = int.from_bytes(power, "little", signed=True)               
                self.powers[i].append(self.convert_power(power))
                del self.powers[i][0]
                
                # Update the timestamp
                self.time = datetime.datetime.now()


    # Publishes the current message data that exists
    def publish_msg (self):
    
        # Get the average data in the message
        for i in range(6):
            self.message.velocities[i]  = sum(self.velocities[i]) / float(STORED_DATA_LEN)
            self.message.powers[i]      = sum(self.powers[i]) / float(STORED_DATA_LEN)
            self.message.rpms[i]        = self.convert_rpm(self.message.velocities[i])
            self.message.velocities[i]  = self.convert_rpm_to_vel (self.message.rpms[i])


        # Publish the data
        self.publisher.publish(self.message)

    
    # Clears the current message if nothing has happened in a while
    def clear_msg (self):
    
        # Check if the last message was a while ago
        if (datetime.datetime.now() - self.time).total_seconds() > 0.5:
            # Clear the message
            self.message = WheelData()
            
            # Reset the averages
            for i in range(6):
                for j in range(STORED_DATA_LEN):
                    self.velocities[i][j] = 0
                    self.powers[i][j] = 0
            
     
    # Converts the value of the velocity to something sensible     
    # Converts a signed integer into a float
    def convert_vel (self, value: int) -> float:
        return value / 32768.0
        
        
    # Converts the value of the power to something sensible
    # Converts a signed integer into a float
    def convert_power (self, value: int) -> float:
        return value / 32768.0 if value > 0 else value / -32768.0

    # Converts a raw velocity to an RPM
    def convert_rpm (self, value: int) -> float:
        return value * ENCODER_TO_RPM

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
