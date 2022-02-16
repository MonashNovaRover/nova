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
CREATION:	15/02/2021
EDITED:		16/02/2021
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# Include all ROS dependencies
import rclpy, json
from rclpy.node import Node
from core.srv import ScienceCommand

# Include utilities for publishing CAN data
from coms_utils.can_interface import CANTransceiver

# Standard CAN ID
CAN_ID = 0


# New looking for CMD
# WILL NEED TO UPDATE THIS
TARGET_USE_CMD = {
    "payload": True,
    "hydraprobe": False,
    "kiln": False
}

ACTION_CMD_ID = {
    "scoop": "8",
    "linear_actuator": "9",
}


# Old Target Dictionary

TARGET_DICT =  {
    "payload": "0",
    "hydraprobe": "1",
    "kiln": "2"
}

ACTION_DICT = {
    "start": "0",
    "scoop": "1",
    "linear_actuator": "5",
    "actuation_top": "6",
    "actuation_bottom": "7",
    "distance_sensor": "A",
    "distance_limit": "B",
    "distance_sensor_reading": "C",
    "hydraprobe_limits": "1",
    "hydraprobe": "2",
    "hydraprobe_top": "3",
    "hydraprobe_bottom": "4",
    "kiln_lid": "1",
    "kiln_mixer": "2",
    "kiln_heater": "3",
}

ARGUMENT_DICT = {
    "forward": "0",
    "reverse": "1",
    "up": "0",
    "down": "1",
    "true": "1",
    "false": "0",
    0: "0",
    1: "1",
    2: "2",
    3: "3",
    4: "4",
    5: "5",
    6: "6",
    7: "7",
    8: "8",
    9: "9",
    10: "A",
    11: "B",
    12: "C",
    13: "D",
    14: "E",
    15: "F"
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

    target_code = TARGET_DICT[target]
    action_code = ACTION_DICT[action]
    argument_codes = []
    for argument in args: 
        try:
            argument_codes.append(ARGUMENT_DICT[args[argument]])
        except KeyError:
            hex = format(args[argument], 'x')
            argument_codes.append(hex.zfill(2))

    code = target_code + action_code + "".join(argument_codes)
    code = code.ljust(6, "0")
    return code


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
        self.can = CANTransceiver(arbitration_id=CAN_ID, channel="can1")


    '''
    Description of callback function
    TODO:
    Fix the CMD stuff - we will need to design a better solution to this
    '''
    def callback_func(self, request, response):

        json_data = json.loads(request.command)

        # Check if command is CMD or PICS
        # If CMD
        if TARGET_USE_CMD[json_data["target"]]:
            command = "00"

            # Update the CAN ID
            new_id = ACTION_CMD_ID[json_data["action"]] + ARGUMENT_DICT[json_data["args"]["direction"]]
            self.can.arbitration_id = int(new_id, 16)


        # If PICS
        else:
            # Parses the command
            command = parse_command()
            print("Executing Command: %s" % command)

            # Update the CAN ID
            self.can.arbitration_id = CAN_ID

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