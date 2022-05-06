#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 receiver code for the EMC publisher
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: EMC_publisher
TOPICS:
  - /science/EMC_data  [WheelData]   [Published]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	science
AUTHOR(S):	Kelly Huang
CREATION:	05/05/2022
EDITED:		
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from typing import Union, List
from coms_utils.uart_interface import CANReciever

import rclpy
import time
from rclpy.node import Node

from core.msg import EMCData 
    
# The EMC CAN arbitration IDs
EMC_IDS = [0x410, 0x420, 0x430, 0x440] #one for each reading?
NUM_READINGS = 4

class EMCPublisher(Node):

    # Stores the current message values
    message: EMCData = EMCData()

    # Main constructor
    def __init__(self):

        super().__init__('EMC_publisher')

        # Print initialisation information
        print("Initialising the EMC Publisher class.")

        # Store the starting time
        self.t = time.time()
    
        # Create the CAN network
        self.cans = [CANReceiver(channel="can1", filter_ids=[EMC_IDS[i]], receive_timeout=1, receive_fmt="<h", bitrate=500000) for i in range(NUM_READINGS)]

        # Create the publisher
        self.publisher_ = self.create_publisher(EMCData, '/science/EMC_data', 10)
        
        # Create the timer
        self.publisher_timer = self.create_timer(3, self.publish_values)

    # Unpacks CAN data and publishes it
    def publish_values(self):

        # Loop through each CAN line and receive data
        t = time.time()
        for i in range(NUM_READINGS):

            # Cache for an errors that come about
            try:
                can_msg = self.cans[i].receive()
                
                # If a message exists
                if can_msg:
                    # Read the velocity data
                    (temperature, pressure, humidity, wind) = self.cans[i].unpack(can_msg.data)
                    # Update the timestamp
                    self.t = time.time()
            
            # In case of an eror, just skip and continue
            except:
                continue

        # Publish message
        # Get the average data in the message DO WE NEED THIS????
        for i in range(NUM_READINGS):
            self.message.temperature[i] = self.temperature[i]
            self.message.pressure[i] = self.pressure[i]
            self.message.humidity[i] = self.humidity[i]
            self.message.wind[i] = self.wind[i]

        # Check for invalid data, reset the message
        if not self.valid:
            self.message = EMCData()

        # Publish the data
        self.publisher.publish(self.message)

    
    # Clears the current message if nothing has happened in a while
    def clear_msg (self):
        
        # Check if the last message was a while ago
        if time.time() - self.t > 0.5:
            # Clear the message
            self.message = EMCData()

def main(args=None):
     # Create the publisher
    rclpy.init(args = args)
    publisher = EMCPublisher()
    rclpy.spin(publisher)

    # Clean up when complete
    publisher.destroy_node()
    rclpy.shutdown()


# Called when the script executes
if __name__=="__main__":
    main()

