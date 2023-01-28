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
# Import plotting tools
from math import pi, sin, cos
import json
from matplotlib import pyplot as plt
import matplotlib.animation as animation
# This is the main class that plots data

fig = plt.figure()
def draw_line(p1, p2, color):
    plt.plot([p1[0], p2[0]], [p1[1], p2[1]], color=color)

def draw_rover(wheel_locations, wheel_angles, wheel_length):
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

def anim(i):
    plt.cla()
    complete = False
    while not complete:
        try:
            with open('wheel_data.json', 'r') as f:
                wheel_data = json.load(f)
            complete = True
        except:
            pass

    print(wheel_data)
    draw_rover(wheel_data["loc"], wheel_data["ang"], 0.4)
    plt.axis('off')
    plt.xlim([-5,5])
    plt.ylim([-5,5])
# Main function for setting up the ROS node
# This code is called when 'python3' is used to run the script

ani = animation.FuncAnimation(fig, anim, interval=round(1000/30))
plt.show()