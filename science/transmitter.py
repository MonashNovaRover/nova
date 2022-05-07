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
EDITED:		06/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include all ROS dependencies
import rclpy, json
from rclpy.node import Node
from core.srv import ScienceCommand

# Include utilities for publishing CAN data
from coms_utils.can_interface import CANTransceiver
import json

# Standard CAN ID
CAN_ID = 0

TARGET_DICT =  {
    "platform": "000",
    "microscope": "000",
    "internals": "001",
    "spectrometer": "001"
}

ACTION_DICT = {
    
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
    "up": "00",
    "down": "01",
    "true": "01",
    "false": "00",
}
'''
Function description
'''
def parse_command(command_dict):
    """
    Command dict requirements.
    * must fill arguments from left to right
    * if not in args dict must be speed or value of range [0, 255] 
    """

    target = command_dict["target"]
    action = command_dict["action"]
    args = command_dict["args"]

    print(target, action, args)
    target_code = TARGET_DICT[target]
    action_code = ACTION_DICT[action]
    argument_codes = []
    for argument in args: 
        if argument == "scoop_id":
            argument_codes.append(args[argument])
        else:
            try:
                arg = ARGUMENT_DICT[args[argument]]
            except KeyError:  # if not in dict, assume value is just a number of two hex digits
                hex = format(int(args[argument]), 'x')
                arg = hex.zfill(2)
            argument_codes.append(arg)

    code = action_code + "".join(argument_codes)
    code = code.ljust(8, "0")
    return (target_code, code)


'''
Description of science class
'''
class ServiceNode(Node):

    def __init__(self):
        # Initialise the node
        super().__init__('science_transmitter')

        # Start the service with the callback
        self.service = self.create_service(ScienceCommand, '/science/transmitter', self.callback_func)

        # Sets up the CAN transceiver interface with the correct ID and channels
        try:
            self.can = CANTransceiver(arbitration_id=CAN_ID, channel="can1")
        
        # CAN does not exist - show error
        except:
            print("\033[1;91m\nERROR: Unable to find can1 network.\033[0m")
            exit()

    '''
    Description of callback function

    '''
    def callback_func(self, request, response):

        json_data = json.loads(request.command)

        command = "00"
        
        # Parses the command
        id, command = parse_command(json_data)
        print("Executing Command: %s" % command)

        # Update the CAN ID
        self.can.arbitration_id = int(id, 16)

        # Execute the CAN command
        response.success = self.can.transmit(bytearray.fromhex(command))

        # Return the response
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
