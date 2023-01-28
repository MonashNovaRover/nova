#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This scripts emulates the GPS and IMU and attempts to send ROS messages
    across to the base station GUI for testing in MapBox.
This does not connect with the actual devices, nor does it actually change
    any of the data stored.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gps_tester_pub
TOPICS:
  - /electronics/rover_pose_gps  [RoverPoseGPS]   [Published]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	electronics
AUTHOR(S):	Harrison Verrios
CREATION:	21/02/2021
EDITED:		26/02/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""


import rclpy
from rclpy.node import Node
import time, math, random, sys

# Get the RoverPose message type
from core.msg import RoverPoseGPS

# This class handles all movement and testing of the GPS
class GPS_Test (Node):

    # Constants
    FREQUENCY = 5 # Rate of change per second

    # Drive constants
    MOVE_SPEED = 0.00001

    # Pitch constants
    PITCH_SPEED = 5.0
    PITCH_MIN = -30
    PITCH_MAX = 30

    # Roll constants
    ROLL_SPEED = 3.0
    ROLL_MIN = -20
    ROLL_MAX = 20

    # Yaw constants
    YAW_SPEED = 5.0
    YAW_MIN = -180
    YAW_MAX = 180

    # Initialises the ROS messages and nodes
    def __init__(self, lat=38.4063, long=110.7918):
        super().__init__('gps_tester_pub')
        print("Initialising ROS GPS/IMU Tester")

        # Message Type, Topic Name, Quality of Service 
        self.pub = self.create_publisher(RoverPoseGPS, '/electronics/rover_pose_gps', 10)

        # Set the starting variables
        self.run = True
        self.latitude = lat
        self.longitude = long

        self.pitch = 0
        self.pitch_target = 0
        self.pitch_flag = True

        self.roll = 0
        self.roll_target = 0
        self.roll_flag = True

        self.yaw = 0
        self.yaw_target = 0
        self.yaw_flag = True

        # Stores the previous position
        self.prevPose = RoverPoseGPS()

        # Start the randomisers
        self.Randomise("pitch")

        # Run the program
        self.Run()

    # Runs a program for the GPS
    def Run(self):
        # Repeat while running
        while self.run:
            # Adjust the values of IMU and GPS
            self.ChangeValue("pitch")
            self.ChangeValue("roll")
            self.ChangeValue("yaw")
            self.ChangeGPS()

            # Create the ROS message
            msg = RoverPoseGPS()
            msg.valid = True
            msg.latitude = float(self.latitude)
            msg.longitude = float(self.longitude)
            msg.pitch = float(self.pitch)
            msg.roll = float(self.roll)
            msg.yaw = float(self.yaw)

            # Output the ROS message
            self.get_logger().info("Publishing data...")
            self.pub.publish(msg)
            
            # Store the previous message
            self.prevPose = msg

            # Sleep for some time period
            time.sleep(1.0/self.FREQUENCY)

    # Increases the GPS
    # Use the yaw value as a direction and then increase based on this
    def ChangeGPS(self):
        self.latitude += math.cos(math.radians(self.yaw)) * (self.MOVE_SPEED / self.FREQUENCY)
        self.longitude += math.sin(math.radians(self.yaw)) * (self.MOVE_SPEED / self.FREQUENCY)

    # Changes a value depending on where it is towards target value
    # Takes in a target, flag and speed and then is able to adjust the value
    def ChangeValue(self, name):
        value = getattr(self, name)
        target = getattr(self, name + "_target")
        flag = getattr(self, name + "_flag")
        speed = getattr(self, name.upper() + "_SPEED")

        # Checks to see if it has not reached the target yet
        if (value < target and flag) or (value > target and not flag):
            setattr(self, name, value + (speed / self.FREQUENCY * self.GetFlagMultiplier(flag)))
        # If it has reached the target, re-randomise
        else:
            self.Randomise(name)

    # Sets the Yaw based on the change in direction
    def GetHeading (self):
        return math.degrees(math.atan((self.latitude - self.prevPose.latitude) / (self.longitude - self.prevPose.longitude))) + 180

    # Randomises a target value
    def Randomise (self, name):
        target = random.uniform(getattr(self, name.upper() + "_MIN"), getattr(self, name.upper() + "_MAX"))
        setattr(self, name + "_target", target)
        setattr(self, name + "_flag", getattr(self, name) < target)

    # Returns a float -1 or 1 depending i/electronics/rover_pose_gps a flag is True(1) or False(-1)
    def GetFlagMultiplier (self, flag):
        if flag:
            return 1.0
        else:
            return -1.0



# Main function for setting up the ROS node  
def main (args = None):
    rclpy.init(args = args)
    # Allow for input for starting coordinates
    if len(sys.argv) > 2:
        publisher = GPS_Test(float(sys.argv[1]), float(sys.argv[2]))
    else:
        # Runs the program
        publisher = GPS_Test()
    rclpy.spin(publisher)

    publisher.destroy_node()
    rclpy.shutdown()

# This code is called when 'python3' is used to run the script   
if __name__ == "__main__":
    main()
