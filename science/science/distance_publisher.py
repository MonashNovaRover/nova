#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 publisher code for 
    the distance sensor.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 
PACKAGE:     science 
AUTHOR(S):   Harrison Verrios
CREATION:    11/05/2022
EDITED:      15/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""  

from coms_utils.can_interface import CANReceiver

import rclpy
import time
from rclpy.node import Node

from core.msg import DistanceData 
    
# The ID for the CAN frame
CAN_ID = 0x004

# The max time [s] if no message has been received
MAX_TIMEOUT = 5.0


# The distance sensor publisher
class DistancePublisher(Node):

    # Stores the current message value
    message: DistanceData = DistanceData()


    # Main constructor
    def __init__(self):

        super().__init__('distance_publisher')

        # Print initialisation information
        self.get_logger().warning("\033[92;1mInitialising the Distance Sensor Publisher class.\033[0m")

        # Store the starting time
        self.t = time.time()
    
        # Create the CAN network
        self.can = CANReceiver(channel="can1", filter_ids=[CAN_ID], receive_timeout=MAX_TIMEOUT, receive_fmt=">H", bitrate=500000)

        # Create the publisher over the network
        self.publisher = self.create_publisher(DistanceData, '/science/distance_data', 10)
        
        # Create the timer to read data
        self.create_timer (0.01, self.publish_values)



    # Unpacks CAN data and publishes it
    def publish_values(self):

        # Cache for an errors that come about
        try:
            # Attempt to receive data
            can_msg = self.can.receive()
            
            # If a message exists
            if can_msg != None:

                # Get the distance data
                distance = self.can.unpack(can_msg.data)[0]

                # Store the distance data
                # Convert from mm to cm
                self.message.distance = float(distance) / 10.0

                # Update the valid flag
                self.message.valid = True

                # Update the timestamp
                self.t = time.time()
        
        # In case of an error, just continue
        except:

            # Clear the message
            self.message = DistanceData()

        # Check if the time has exceeded a delay
        if time.time() - self.t > MAX_TIMEOUT:

            # Clear the message
            self.message = DistanceData()

        # Publish the message
        self.publisher.publish(self.message)



# The main code that executes when starting
def main(args=None):

    # Create the publisher
    rclpy.init(args = args)
    publisher = DistancePublisher()
    rclpy.spin(publisher)

    # Clean up when complete
    publisher.destroy_node()
    rclpy.shutdown()


# Called when the script executes
if __name__=="__main__":
    main()

