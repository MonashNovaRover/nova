#!/usr/bin/env python3

'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class receives data from the PID controller and
    plots the data on a graph.
It helps to tune the PID constants by showing the
    actual velocity of each of the CMDs.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: pid_tuner
TOPICS:
  - /control/cmd_feedback   [CMDFeedback]   [Subscribed]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Harrison Verrios
CREATION:	06/01/2022
EDITED:		06/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

# Import al ROS 2 packages
import rclpy
from rclpy.node import Node

# Import the required message
from core.msg import CMDFeedback

# Import plotting tools
from matplotlib import pyplot as plt


# This is the main class that plots data
class CMDPlotter (Node):

    # Main constructor called when the class is initialised
    def __init__(self) -> None:
        super().__init__('cmd_plotter')

        # Store the current device
        self.device = "00"

        # Create the subscriber
        self.subscription = self.create_subscription(CMDFeedback, '/control/cmd_feedback', self.feedback_callback, 10)
        print("Initialised the Feedback Plotter")


    # The callback function when the message is received
    def feedback_callback (self, msg):

        # Check if device is different
        device = "%d%d" % (msg.bus, msg.id)
        if device != self.device:

            # Update the device
            self.device = device

            # Store the raw arrays
            self.rpms = []
            self.powers = []
            self.times = []

        # Add data to the arrays
        self.rpms.append(msg.rpm)
        self.powers.append(msg.power)
        self.times.append(msg.time)

        # Clear the plot
        plt.clf()
        
        # Plot the data
        plt.plot(self.times, self.rpms, label = "RPM")
        plt.plot(self.times, self.powers, label = "Power")

        # Improve the graph
        plt.title("CMD Feedback. Bus: %d, ID: %d" % (msg.bus, msg.id))
        plt.legend()
        plt.grid()
        plt.ylim(-100, 100)
        plt.xlim(self.times[0], self.times[0] + 10000)
        plt.xlabel("Time [ms]")
        plt.draw()
        plt.pause(0.001)
        
    

# Main function for setting up the ROS node
def main (args = None):
    rclpy.init(args = args)
    subscriber = CMDPlotter()
    rclpy.spin(subscriber)

    subscriber.destroy_node()
    rclpy.shutdown()


# This code is called when 'python3' is used to run the script
if __name__ == '__main__':
    main()