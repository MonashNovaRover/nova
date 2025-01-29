#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This script emulates battery state messages that get
published over ROS

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: battery_tester_pub
TOPICS:
  - /electronics/battery_state  [BatteryState]   [Published]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Harrison Verrios
CREATION:	26/02/2022
EDITED:		07/06/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node
import time, random

# Get the message type
from sensor_msgs.msg import BatteryState

# This class handles all movement and testing of the battery state
class BatteryTest(Node):

    # Constants
    FREQUENCY = 10  # Rate of change per second

    # Data transmission rates
    VOLTAGE_SPEED = 0.1
    VOLTAGE_MIN = 11.0
    VOLTAGE_MAX = 12.6

    CURRENT_SPEED = 0.05
    CURRENT_MIN = -2.0
    CURRENT_MAX = 2.0

    # Initialises the ROS messages and nodes
    def __init__(self):
        super().__init__('battery_tester_pub')
        print("Initialising ROS Battery Tester")

        # Message Type, Topic Name, Quality of Service 
        self.pub = self.create_publisher(BatteryState, '/electronics/battery_state', 10)

        # Set the starting variables
        self.run = True

        self.voltage = 0.0
        self.voltage_target = 0.0
        self.voltage_flag = True

        self.current = 0.0
        self.current_target = 0.0
        self.current_flag = True

        # randomise
        self.randomise("voltage")
        self.randomise("current")

        # Run the program
        self.run_test()

    # Runs a program for the battery state
    def run_test(self):
        # Repeat while running
        while self.run:
            # Adjust the values
            
            # Create the ROS message
            msg = BatteryState()
            msg.voltage = self.voltage
            msg.current = self.current

            # Output the ROS message
            self.get_logger().info("Publishing battery state data...")
            self.pub.publish(msg)

            # Sleep for some time period
            time.sleep(1.0 / self.FREQUENCY)

    # Changes a value depending on where it is towards target value
    # Takes in a target, flag and speed and then is able to adjust the value
    def change_value(self, name):
        value = getattr(self, name)
        target = getattr(self, name + "_target")
        flag = getattr(self, name + "_flag")
        speed = getattr(self, name.upper() + "_SPEED")

        # Checks to see if it has not reached the target yet
        if (value < target and flag) or (value > target and not flag):
            setattr(self, name, value + (speed / self.FREQUENCY * self.get_flag_multiplier(flag)))
        # If it has reached the target, re-randomise
        else:
            self.randomise(name)

    # Randomises a target value
    def randomise(self, name):
        setattr(self, name + "_target", random.uniform(getattr(self, name.upper() + "_MIN"), getattr(self, name.upper() + "_MAX")))
        setattr(self, name + "_flag", getattr(self, name) < getattr(self, name + "_target"))

    # Returns a float -1 or 1 depending flag is True(1) or False(-1)
    def get_flag_multiplier(self, flag):
        return 1.0 if flag else -1.0


# Main function for setting up the ROS node  
def main(args=None):
    rclpy.init(args=args)
    publisher = BatteryTest()
    rclpy.spin(publisher)

    publisher.destroy_node()
    rclpy.shutdown()

# This code is called when 'python3' is used to run the script   
if __name__ == "__main__":
    main()
