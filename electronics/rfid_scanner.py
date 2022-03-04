#!/usr/bin/env python3

'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

Reads RFID Data and publishes to a ROS topic

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: pid_tuner
TOPICS:
  - /control/cmd_feedback   [CMDFeedback]   [Subscribed]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	????
CREATION:	06/01/2022
EDITED:		06/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

# Import al ROS 2 packages
import rclpy
from rclpy.node import Node

from std_msgs.msg import String

# This is the main class that plots data
class RFIDNode(Node):

    # Main constructor called when the class is initialised
    def __init__(self) -> None:
        super().__init__('rfid_node')
        self.publisher = self.create_publisher(String, '/sensors/rfid', 10)
        self.timer = self.create_timer(1.0, self.rfid_callback)

    def rfid_callback(self):
        msg = String()
        msg.data = "test"
        self.publisher.publish(msg)
        msg.data = str(input())
        print("hi")
        self.publisher.publish(msg)

# Main function for setting up the ROS node
def main (args = None):
    rclpy.init(args = args)
    node = RFIDNode()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


# This code is called when 'python3' is used to run the script
if __name__ == '__main__':
    main()
