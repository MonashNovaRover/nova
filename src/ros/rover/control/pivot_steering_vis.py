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
import time
from math import sin, cos, pi
from rclpy.node import Node

# Import the required message
from core.msg import PivotWheelData

# Import plotting tools
from matplotlib import pyplot as plt


# This is the main class that plots data
def draw_line(p1, p2, color):
    plt.plot([p1[0], p2[0]], [p1[1], p2[1]], color=color)

def draw_rover(wheel_locations, wheel_angles, wheel_length):
    plt.clf()
    draw_line(wheel_locations[0], wheel_locations[1], 'k')
    draw_line(wheel_locations[0], wheel_locations[2], 'k')
    draw_line(wheel_locations[1], wheel_locations[3], 'k')
    draw_line(wheel_locations[2], wheel_locations[3], 'k')
    plot_wheel_dir(wheel_locations, wheel_angles, wheel_length)

def plot_wheel_dir(wheel_locations, wheel_angles, wheel_length):
    for i in range(len(wheel_locations)):
        draw_line((wheel_locations[i][0] + cos(wheel_angles[i] + pi/2)*wheel_length/2,
                   wheel_locations[i][1] + sin(wheel_angles[i] + pi/2)*wheel_length/2),
                  (wheel_locations[i][0] - cos(wheel_angles[i] + pi/2) * wheel_length / 2,
                   wheel_locations[i][1] - sin(wheel_angles[i] + pi/2) * wheel_length / 2), 'b')



class PivotWheelPlotter (Node):

    # Main constructor called when the class is initialised
    def __init__(self, wheel_base_sep: float, wheel_length_sep: float, wheel_length: float) -> None:
        super().__init__('pivot_wheel_plotter')

        self.wheel_locations = [(0,wheel_length_sep), (wheel_base_sep, wheel_length_sep), (0, 0), (wheel_base_sep, 0)]
        self.wheel_vel = [0,0,0,0]
        self.wheel_angles = [0,0,0,0]
        self.wheel_length = wheel_length
        self.delta = 1/30
        self.timer = self.create_timer(self.delta, self.timer_callback)

        # Create the subscriber
        self.subscription = self.create_subscription(PivotWheelData, '/control/pivot_wheel_data', self.wheel_data_callback, 10)
        print("Initialised the Feedback Plotter")

    # The callback function when the message is received
    def wheel_data_callback (self, msg):
        self.wheel_vel = msg.velocities
        self.wheel_angles = msg.angles

    def timer_callback(self):
        for i, _ in enumerate(self.wheel_locations):
            self.wheel_locations[i][0] += self.wheel_vel[i]*cos(self.wheel_angles[i] + pi/2) * self.delta
            self.wheel_locations[i][1] += self.wheel_vel[i] * cos(self.wheel_angles[i] + pi / 2) * self.delta
        draw_rover(self.wheel_locations, self.wheel_angles, self.wheel_length)

# Main function for setting up the ROS node
def main (args = None):
    rclpy.init(args = args)
    wheel_plotter = PivotWheelPlotter(1, 2)
    rclpy.spin(wheel_plotter)

    wheel_plotter.destroy_node()
    rclpy.shutdown()


# This code is called when 'python3' is used to run the script
if __name__ == '__main__':
    #main()
    wheel_length_sep = 2
    wheel_base_sep = 1
    wheel_angles = [0,0,0,0]
    wheel_locations = [(0, wheel_length_sep), (wheel_base_sep, wheel_length_sep), (0, 0), (wheel_base_sep, 0)]
    draw_rover(wheel_locations, wheel_angles, wheel_base_sep/2)
    plt.axis('equal')
    plt.axis('off')
    plt.xlim([-10,10])
    plt.ylim([-10,10])
    plt.show()
