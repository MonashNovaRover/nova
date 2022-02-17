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
WHEEL_IDS = [0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6]


# Main Wheel Publisher class
class WheelPublisher (Node):

    # Stores the current message values
    message: WheelData = WheelData()

    # Constructor sets up the publisher
    def __init__ (self):

        # Print initialisation information
        print("Initialising the Wheel Publisher class.")

        # Store the starting time
        self.time = datetime.datetime.now()

        # Set up the CAN interface for each wheel id
        self.cans = []
        for i in range(6):
            self.cans.append(CANReceiver(channel="can0", filter_ids=[WHEEL_IDS[i]], receive_timeout=0.1))

        # Create the publisher
        super().__init__("wheel_publisher")
        self.publisher = self.create_publisher(WheelData, "/electronics/wheel_data", 10)

        # Create a time to constantly loop and check for data
        self.read_timer = self.create_timer(0.1, self.read_callback)

        # Create a timer to publish the current data
        self.pub_timer = self.create_timer(0.1, self.publish_msg)

        # Create a timer to clear the current message data if nothing has happened
        self.clear_timer = self.create_timer(1.0, self.clear_msg)

        

    
    # Method that looks for any changes in the data from the CAN lines
    def read_callback (self):
        # Loop through each can line and look
        for i in range(6):
            can_msg = self.cans[i].receive()

            # If a message exists
            if can_msg:
                self.message.velocities[i] = int.from_bytes(can_msg, "big")
                
                self.time = datetime.datetime.now()

    # Publishes the current message data that exists
    def publish_msg (self):

        # Publish
        self.publisher.publish(self.message)

    
    # Clears the current message if nothing has happened in a while
    def clear_msg (self):
        # Check if the last message was a while ago
        if (datetime.datetime.now() - self.time).total_seconds() > 1.0:
            # Clear the message
            self.message = WheelData()



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