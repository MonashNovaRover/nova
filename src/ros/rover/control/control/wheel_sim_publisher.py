#!/usr/bin/env python3

'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class receives data from the drive code for
    wheel angles and velocities, and simulates
    location of wheels.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: pid_tuner
TOPICS:
  - /control/drive   [PivotWheelData]   [Subscribed]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Taaj Street
CREATION:	12/01/2023
EDITED:		12/01/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

# Import al ROS 2 packages
import rclpy
import json
import time
from math import sin, cos, pi, atan2
from rclpy.node import Node

# Import the required message
from core.msg import PivotWheelData, Point2D

class WheelSimLocations (Node):

    # Main constructor called when the class is initialised
    def __init__(self, wheel_base_sep: float, wheel_length_sep: float) -> None:
        super().__init__('pivot_wheel_plotter')

        self.wheel_locations = [[0,wheel_length_sep], [wheel_base_sep, wheel_length_sep], [0, 0], [wheel_base_sep, 0]]
        self.wheel_vel = [0,0,0,0]
        self.wheel_angles = [0,0,0,0]
        self.delta = 1/30
        self.heading = 0

        # Create the subscriber
        self.subscription = self.create_subscription(PivotWheelData, '/control/pivot_wheel_data', self.wheel_data_callback, 10)
        self.timer = self.create_timer(self.delta, self.update_pos)

    def update_heading(self):
        dx = self.wheel_locations[2][0] - self.wheel_locations[0][0]
        dy = self.wheel_locations[2][1] - self.wheel_locations[0][1]
        self.heading = atan2(dy,dx) - pi/2
    # The callback function when the message is received
    def wheel_data_callback (self, msg):
        self.wheel_vel = msg.velocities
        self.wheel_angles = msg.angles

    def update_pos(self):
        for i, _ in enumerate(self.wheel_locations):
            self.wheel_locations[i][0] += self.wheel_vel[i]*cos(self.wheel_angles[i]+pi/2 + self.heading) * self.delta
            self.wheel_locations[i][1] += self.wheel_vel[i] * sin(self.wheel_angles[i]+pi/2 + self.heading) * self.delta

        self.update_heading()

        with open('wheel_data.json', 'w') as f:
                json.dump({"loc": self.wheel_locations, "ang": [float(i) + self.heading for i in self.wheel_angles]}, f)

# Main function for setting up the ROS node
def main (args = None):
    rclpy.init(args = args)
    wheel_plotter = WheelSimLocations(0.8, 0.85)
    rclpy.spin(wheel_plotter)

    wheel_plotter.destroy_node()
    rclpy.shutdown()


# This code is called when 'python3' is used to run the script
if __name__ == '__main__':
    main()
