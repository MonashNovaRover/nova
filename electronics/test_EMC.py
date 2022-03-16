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
    FREQUENCY = 1.0 # Rate of change per second

    # Hydraprobe Temperature
    HYDRA_TEMP_SPEED = 0.12
    HYDRA_TEMP_MIN = 20
    HYDRA_TEMP_MAX = 24

    # Kiln Temperature
    KILN_TEMP_SPEED = 1.0
    KILN_TEMP_MIN = 20
    KILN_TEMP_MAX = 110

    # Moisture
    MOISTURE_SPEED = 0.00137
    MOISTURE_MIN = 0.0
    MOISTURE_MAX = 0.2
    
    # Conductivity
    CONDUCTIVITY_SPEED = 0.00056
    CONDUCTIVITY_MIN = 0.0
    CONDUCTIVITY_MAX = 0.01

    # Initialises the ROS messages and nodes
    def __init__(self):
        super().__init__('test_emc_pub')
        print("Initialising ROS Radio Tester")

        # Message Type, Topic Name, Quality of Service 
        self.pub = self.create_publisher(HydraprobeData, '/science/hydraprobe_data', 10)

        # Set the starting variables
        self.run = True

        self.hydra_temp = 0
        self.hydra_temp_target = 0
        self.hydra_temp_flag = True

        self.kiln_temp = self.KILN_TEMP_MIN
        self.kiln_temp_target = 0
        self.kiln_temp_flag = True

        self.moisture = 0
        self.moisture_target = 0
        self.moisture_flag = True

        self.conductivity = 0
        self.conductivity_target = 0
        self.conductivity_flag = True

        # Stores the previous data
        self.prevData = HydraprobeData()

        # Start the randomisers
        self.Randomise("hydra_temp")

        # Run the program
        self.Run()

    # Runs a program for the Radio
    def Run(self):
        # Repeat while running
        while self.run:
            # Adjust the values
            self.ChangeValue("hydra_temp")
            self.ChangeValue("kiln_temp")
            self.ChangeValue("moisture")
            self.ChangeValue("conductivity")

            # Create the ROS message
            msg = HydraprobeData()
            msg.temperature = float(self.hydra_temp)
            msg.moisture = float(self.moisture)
            msg.conductivity = float(self.conductivity)
            #msg.kiln = float(self.kiln_temp)

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