#!/usr/bin/env python3

import rclpy
from rclpy.node import Node
from example_interfaces.srv import AddTwoInts
import json

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
    "kiln_pump": "4",
    "kiln_temperature_reading": "A"

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
    print(code)
    return code


class ServiceNode(Node):
    def __init__(self):
        super().__init__('transmitter_service')
        self.service = self.create_service(ScienceCommand, 'science_transmitter', self.callback_func)
    
    def callback_func(self, request, response):
        command = parse_command(json.load(request))
        print(command)
        return response


def main (args = None):
  rclpy.init(args = args)
  service = ServiceNode()
  rclpy.spin(service)
  rclpy.shutdown()

if __name__ == '__main__':
  main()