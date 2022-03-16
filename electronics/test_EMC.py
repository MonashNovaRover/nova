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
from core.msg import HydraprobeData

# This class handles all movement and testing of the GPS
class RadioTest (Node):

    # Constants
    FREQUENCY = 10.0 # Rate of change per second

    # Data receiving rates
    UV_SPEED = 0.1
    UV_MIN = 5
    UV_MAX = 6

    # Data transmission rates
    LUX_SPEED = 124
    LUX_MIN = 15000
    LUX_MAX = 20000

    # Ping
    TEMP_SPEED = 0.245
    TEMP_MIN = 24.5
    TEMP_MAX = 26.7
    
    # Signal strengths
    PRESSURE_SPEED = 13.5
    PRESSURE_MIN = 101000
    PRESSURE_MAX = 102000

    METHANE_SPEED = 0.012
    METHANE_MIN = 0.0
    METHANE_MAX = 0.1

    WIND_SPEED = 4.23
    WIND_MIN = 1
    WIND_MAX = 20

    # Initialises the ROS messages and nodes
    def __init__(self):
        super().__init__('emc_pub')
        print("Initialising ROS Radio Tester")

        # Message Type, Topic Name, Quality of Service 
        self.pub = self.create_publisher(HydraprobeData, '/science/hydraprobe_data', 10)

        # Set the starting variables
        self.run = True

        self.uv = 0
        self.uv_target = 0
        self.uv_flag = True

        self.lux = 0
        self.lux_target = 0
        self.lux_flag = True

        self.temp = 0
        self.temp_target = 0
        self.temp_flag = True

        self.pressure = 0
        self.pressure_target = 0
        self.pressure_flag = True

        self.methane = 0
        self.methane_target = 0
        self.methane_flag = True

        self.wind = 0
        self.wind_target = 0
        self.wind_flag = True

        # Stores the previous data
        self.prevData = HydraprobeData()

        # Start the randomisers
        self.Randomise("uv")
        self.Randomise("lux")
        self.Randomise("temp")
        self.Randomise("wind")
        self.Randomise("methane")
        self.Randomise("pressure")

        # Run the program
        self.Run()

    # Runs a program for the Radio
    def Run(self):
        # Repeat while running
        while self.run:
            # Adjust the values
            self.ChangeValue("lux")
            self.ChangeValue("uv")
            self.ChangeValue("temp")
            self.ChangeValue("methane")
            self.ChangeValue("pressure")
            self.ChangeValue("wind")

            # Create the ROS message
            msg = HydraprobeData()
            msg.temperature = float(self.temp)
            msg.lux = float(self.lux)
            msg.wind = float(self.wind)
            msg.methane = float(self.methane)
            msg.pressure = float(self.pressure)
            msg.uv = float(self.uv)

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