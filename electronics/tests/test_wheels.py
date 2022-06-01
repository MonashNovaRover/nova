#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This script emulates wheel data messages that get
published over ROS

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: radio_tester_pub
TOPICS:
  - /electronics/wheel_data  [WheelData]   [Published]
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

# Get the message type
from core.msg import WheelData

# This class handles all movement and testing of the GPS
class WheelTest (Node):

    # Constants
    FREQUENCY = 10 # Rate of change per second

    # Data receiving rates
    VEL_SPEED = 0.1
    VEL_MIN = 0.6
    VEL_MAX = 1.0

    # Data transmission rates
    POWER_SPEED = 0.4
    POWER_MIN = 0.35
    POWER_MAX = 0.95

    # Initialises the ROS messages and nodes
    def __init__(self):
        super().__init__('wheels_tester_pub')
        print("Initialising ROS Radio Tester")

        # Message Type, Topic Name, Quality of Service 
        self.pub = self.create_publisher(WheelData, '/electronics/wheel_data', 10)

        # Set the starting variables
        self.run = True

        self.vel = [0] * 6
        self.vel_target = [0] * 6
        self.vel_flag = [True] * 6

        self.power = [0] * 6
        self.power_target = [0] * 6
        self.power_flag = [True] * 6

        # Stores the previous data
        self.prevData = WheelData()
        
        # Randomise
        for i in range(6): self.Randomise("vel", i)
        for i in range(6): self.Randomise("power", i)

        # Run the program
        self.Run()

    # Runs a program for the Radio
    def Run(self):
        # Repeat while running
        while self.run:
            # Adjust the values
            self.ChangeValue("vel")
            self.ChangeValue("power")

            # Create the ROS message
            msg = WheelData()
            for i in range(6): msg.powers[i] = self.power[i]
            for i in range(6): msg.velocities[i] = self.vel[i]
            for i in range(6): msg.rpms[i] = self.vel[i] * 60.0 / (2.0 * 3.14159 * 0.122)

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
        for i in range(6):
            value = getattr(self, name)[i]
            target = getattr(self, name + "_target")[i]
            flag = getattr(self, name + "_flag")[i]
            speed = getattr(self, name.upper() + "_SPEED")
            min = getattr(self, name.upper() + "_MIN")
            max = getattr(self, name.upper() + "_MAX")

            # Checks to see if it has not reached the target yet
            if (value < target and flag) or (value > target and not flag):
                values = getattr(self, name)
                values[i] = value + (speed / self.FREQUENCY * self.GetFlagMultiplier(flag))
                setattr(self, name, values)
            # If it has reached the target, re-randomise
            else:
                self.Randomise(name, i)

            # Check for min and max
            #if (value < min): setattr(self, name, min)
            #if (value > max): setattr(self, name, max)
        

    # Randomises a target value
    def Randomise (self, name, i: int):
        targets = getattr(self, name.lower() + "_target")
        flags = getattr(self, name.lower() + "_flag")
        targets[i] = random.uniform(getattr(self, name.upper() + "_MIN"), getattr(self, name.upper() + "_MAX"))
        flags[i] = getattr(self, name)[i] < targets[i]
        setattr(self, name + "_target", targets)
        setattr(self, name + "_flag", flags)

    # Returns a float -1 or 1 depending flag is True(1) or False(-1)
    def GetFlagMultiplier (self, flag):
        if flag:
            return 1.0
        else:
            return -1.0



# Main function for setting up the ROS node  
def main (args = None):
    rclpy.init(args = args)
    publisher = WheelTest()
    rclpy.spin(publisher)

    publisher.destroy_node()
    rclpy.shutdown()

# This code is called when 'python3' is used to run the script   
if __name__ == "__main__":
    main()
