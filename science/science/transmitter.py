#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This node interfaces with the science CAN terminals
and is able to send data across the CAN network.
It converts a science command, from the GUI, into a
CAN command to the science payload.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: science_transmitter
SERVICES:
  - /science/science_transmitter   [ScienceCommand]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	science
AUTHOR(S):	Miles Higgins, Harrison Verrios
CREATION:	15/02/2022
EDITED:		15/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include all ROS dependencies
import rclpy
from rclpy.node import Node
from core.srv import ScienceCommand

# Include utilities for publishing CAN data
from coms_utils.can_interface import CANTransceiver

# Include other dependencies
import json, os


'''
Science node class that acts as a ttransmitter for the science data
over the CAN lines. It interfaces with a ROS service that can communicate
the commands.
'''
class ServiceNode(Node):

    def __init__(self):

        # Initialise the node
        super().__init__('science_transmitter')

        # Print initialisation information
        self.get_logger().warning("\033[92;1mInitialising the Science Transmitter class.\033[0m")

        # Get the directory of the JSON file
        directory = os.path.expanduser('~') + "/nova_ws/src/rover/science/configs/commands.json"

        # Reads the command json data
        with open (directory, "r") as file:
            self.archive = json.loads(file.read())
        
        # Store the list of targets
        self.targets = list(self.archive["targets"].keys())

        # Store the list of argument decoding
        self.arg_encoding = self.archive["arg_encoding"]

        # Start the service with the callback
        self.service = self.create_service(ScienceCommand, '/science/transmitter', self.callback_func)

        # Sets up the CAN transceiver interface with the correct ID and channels
        try:
            self.can = CANTransceiver(arbitration_id=0x0, channel="can1", bitrate=500000)
        
        # CAN does not exist - show error
        except:
            self.get_logger().error("\033[91;1m\nTransmitter ERROR: Unable to find can1 network.\033[0m")
            exit()


    '''
    The callback function is execited when a command is received by the transmitter.
    It takes in a request and a response message and outputs the associated
    response from the service.

    '''
    def callback_func(self, request, response):

        # Load the data
        try:
            # Reads the command
            command_data = json.loads(request.command)
            target = command_data["target"].lower()
            action = command_data["action"].lower()
            args = command_data["args"]
            
            # Check for an invalid target entered in the command
            if target not in self.targets:
                raise Exception("Invalid target '%s'." % target)

            # Grab the arbitration ID
            arb_id = self.archive["targets"][target]["hex"]
            self.can.arbitration_id = int(arb_id, 16)

            # Set the target data
            target = self.archive["targets"][target]

            # Check if the action exists
            if action not in target["actions"].keys():
                raise Exception("Invalid action '%s'." % action)

            # Grab the action ID
            action_id = target["actions"][action]["hex"]

            # Set the action target
            action = target["actions"][action]

            # Store an argument id
            arg_id = ""

            # Go through each of the actions and ensure they exist
            for arg in action["args"]:
                # Ensure it exists
                if arg not in args.keys():
                    raise Exception("Missing argument '%s'." % arg)

                # If it does exist, check the decoder
                value = args[arg]

                # Check for non-number argument
                if str(value).lower() not in self.arg_encoding.keys():
                    arg_id += format(int(value), "x").rjust(2, "0")

                # Get the decoded value and add to the list
                else:
                    arg_id += self.arg_encoding[str(value).lower()]

            # Create the command
            command = action_id + arg_id

            # Execute the CAN command
            response.success = self.can.transmit(bytearray.fromhex(command))

            # Print a success
            self.get_logger().warning("\033[1;92m\nTransmitter SUCCESS! Command: %s#%s\033[0m" % (arb_id, command))

        
        
        # If an error occurred
        except Exception as e:
            self.get_logger().error("\033[1;91m\nTransmitter ERROR! Failed to parse science command because:\n\t%s\033[0m" % e)
            
            # Process failed
            response.success = False


        # Return the response data
        return response



# When the script starts, it runs the science class and waits until completion
def main (args = None):
    rclpy.init(args = args)
    service = ServiceNode()
    rclpy.spin(service)
    rclpy.shutdown()


# Called when the script starts
if __name__ == '__main__':
    main()
