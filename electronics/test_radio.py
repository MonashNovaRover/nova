#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This script emulates radio status messages that get
published over ROS

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: radio_tester_pub
TOPICS:
  - /electronics/radio_status  [RadioStatus]   [Published]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Harrison Verrios
CREATION:	26/02/2022
EDITED:		26/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""


import rclpy
from rclpy.node import Node
import time, math, random, sys

# Get the RoverPose message type
from core.msg import RadioStatus

# This class handles all movement and testing of the GPS
class RadioTest (Node):

    # Constants
    FREQUENCY = 1.0 # Rate of change per second

    # Data receiving rates
    RECV_SPEED = 723
    RECV_MIN = 1523
    RECV_MAX = 5045

    # Data transmission rates
    SENT_SPEED = 21
    SENT_MIN = 5
    SENT_MAX = 54

    # Ping
    PING_SPEED = 137
    PING_MIN = 5
    PING_MAX = 253
    
    # Signal strengths
    SIGNAL_SPEED = 0.4
    SIGNAL_MIN = -71.0
    SIGNAL_MAX = -62.0

    # Initialises the ROS messages and nodes
    def __init__(self):
        super().__init__('radio_tester_pub')
        print("Initialising ROS Radio Tester")

        # Message Type, Topic Name, Quality of Service 
        self.pub = self.create_publisher(RadioStatus, '/electronics/radio_status', 10)

        # Set the starting variables
        self.run = True

        self.recv = 0
        self.recv_target = 0
        self.recv_flag = True

        self.sent = 0
        self.sent_target = 0
        self.sent_flag = True

        self.ping = 0
        self.ping_target = 0
        self.ping_flag = True

        self.signal = -65
        self.signal_target = 0
        self.signal_flag = True

        # Stores the previous data
        self.prevData = RadioStatus()

        # Start the randomisers
        self.Randomise("recv")
        self.Randomise("sent")
        self.Randomise("ping")
        self.Randomise("signal")

        # Run the program
        self.Run()

    # Runs a program for the Radio
    def Run(self):
        # Repeat while running
        while self.run:
            # Adjust the values
            self.ChangeValue("recv")
            self.ChangeValue("sent")
            self.ChangeValue("ping")
            self.ChangeValue("signal")

            # Create the ROS message
            msg = RadioStatus()
            msg.recv = int(self.recv)
            msg.sent = int(self.sent)
            msg.ping = int(self.ping)
            msg.signal = int(self.signal)

            # Output the ROS message
            self.get_logger().info("Publishing data...")
            self.pub.publish(msg)
            
            # Store the previous message
            self.prevPose = msg

            # Sleep for some time period
            time.sleep(1.0/self.FREQUENCY)

   
    # Changes a value depending on where it is towards target value
    # Takes in a target, flag and speed and then is able to adjust the value
    def ChangeValue(self, name):
        value = getattr(self, name)
        target = getattr(self, name + "_target")
        flag = getattr(self, name + "_flag")
        speed = getattr(self, name.upper() + "_SPEED")
        min = getattr(self, name.upper() + "_MIN")
        max = getattr(self, name.upper() + "_MAX")

        # Checks to see if it has not reached the target yet
        if (value < target and flag) or (value > target and not flag):
            setattr(self, name, value + (speed / self.FREQUENCY * self.GetFlagMultiplier(flag)))
        # If it has reached the target, re-randomise
        else:
            self.Randomise(name)

        # Check for min and max
        if (value < min): setattr(self, name, min)
        if (value > max): setattr(self, name, max)
        

    # Randomises a target value
    def Randomise (self, name):
        target = random.uniform(getattr(self, name.upper() + "_MIN"), getattr(self, name.upper() + "_MAX"))
        setattr(self, name + "_target", target)
        setattr(self, name + "_flag", getattr(self, name) < target)

    # Returns a float -1 or 1 depending flag is True(1) or False(-1)
    def GetFlagMultiplier (self, flag):
        if flag:
            return 1.0
        else:
            return -1.0



# Main function for setting up the ROS node  
def main (args = None):
    rclpy.init(args = args)
    publisher = RadioTest()
    rclpy.spin(publisher)

    publisher.destroy_node()
    rclpy.shutdown()

# This code is called when 'python3' is used to run the script   
if __name__ == "__main__":
    main()