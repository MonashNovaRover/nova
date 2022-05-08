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
EDITED:		08/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include all ROS dependencies
import rclpy
from rclpy.node import Node
from core.srv import ScienceCommand

# Include utilities for publishing CAN data
from coms_utils.can_interface import CANTransceiver

# Include other dependencies
import json, time

# Standard CAN ID
CAN_ID = 0

TARGET_HEX =  {
    "platform": "000",
    "microscope": "000",
    "internals": "001",
    "spectrometer": "001"
}

ACTION_HEX = {
    "enable": "00",
    "actuation_manual": "01",
    "actuation_top": "02",
    "actuation_bottom": "03",
    "scoops": "04",
    "lens": "05",
    "led": "06",
    "laser": "07",
    "sequence": "01",
    "controls": "02",
    "pumps": "03",
    "rotate": "11",
    "read": "12",
    "led": "13"
}

ARGUMENT_DICT = {
    "forward": "00",
    "reverse": "01",
    "left": "00",
    "right": "01",
    "up": "01",
    "down": "00",
    "True": "01",
    "False": "00",
    "coarse": "00",
    "fine": "01"
}

'''
A list of all arguments that use an integer
between 0 and 255 as the converted data.
'''
INTEGER_ARGS = [
    "id", 
    "time", 
    "speed", 
    "steps"
]


'''
Parses a command from a dictionary and sets up the data
appropriate to the values
'''
def parse_command(command_dict):
    """
    Command dict requirements.
    * must fill arguments from left to right
    * if not in args dict must be speed or value of range [0, 255] 
    """

    # Get the three components of the command
    target = command_dict["target"]
    action = command_dict["action"]
    args = command_dict["args"]

    # Get the codes associated with the target and action
    target_code = TARGET_HEX[target]
    action_code = ACTION_HEX[action]

    # Store the argument data
    argument_data = []
    for argument in args:
        # If the argument is an integer value
        if argument in INTEGER_ARGS:
            hex = format(int(args[argument]), 'x')
        
        # Category argument
        else:
            try:
                # Convert the argument to a hard-coded value
                arg = ARGUMENT_DICT[str(args[argument])]
                argument_data.append(arg)

            # Failed to convert correctly (invalid argument)
            except KeyError:
                print("Invalid argument %s pasrsed.", argument)
                return

    # Construct the code from the data
    data = action_code + "".join(argument_data)
    data = data.ljust(8, "0")

    # Return the target code and data
    return (target_code, data)


'''
Description of science class
'''
class ServiceNode(Node):

    def __init__(self):
        # Initialise the node
        super().__init__('science_transmitter')

        # Reads the command json data
        with open ("commands.json", "r") as file:
            self.archive = json.loads(file.read())
        
        # Store the list of targets
        self.targets = list(self.archive["targets"].keys())

        # Store the list of argument decoding
        self.arg_encoding = self.archive["arg_encoding"]

        # Start the service with the callback
        self.service = self.create_service(ScienceCommand, '/science/transmitter', self.callback_func)

        # Sets up the CAN transceiver interface with the correct ID and channels
        try:
            self.can = CANTransceiver(arbitration_id=CAN_ID, channel="can1")
        
        # CAN does not exist - show error
        except:
            print("\033[1;91m\nTransmitter ERROR: Unable to find can1 network.\033[0m")
            exit()


    '''
    Description of callback function

    '''
    def callback_func(self, request, response):

        # Load the data
        try:
            # Reads the command
            command_data = json.loads(request.command)
            target = command_data["target"]
            action = command_data["action"]
            args = command_data["args"]
            
            # Check for an invalid target entered in the command
            if target not in self.targets:
                raise Exception("Invalid target.")

            # Grab the arbitration ID
            arb_id = self.archive["targets"][target]["hex"]
            self.can.arbitration_id = int(arb_id, 16)

            # Set the target data
            target = self.archive["targets"][target]

            # Check if the action exists
            if action not in target.keys():
                raise Exception("Invalid action.")

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
                    raise Exception("Missing argument %s" % arg)

                # If it does exist, check the decoder
                value = args[arg]

                # Check for invalid argument
                if value not in self.arg_encoding.keys():
                    raise Exception("Invalid argument value %s for %s" % (str(value), arg))

                # Get the decoded value and add to the list
                arg_id += self.arg_encoding[value]

            # Create the command
            command = action_id + arg_id
            print(command)

            # Execute the CAN command
            response.success = self.can.transmit(bytearray.fromhex(command))
        
        
        # If an error occurred
        except Exception as e:
            print("\033[1;91m\nTransmitter ERROR: Failed to parse science command because:\n\t%s.\033[0m" % e)
            
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
