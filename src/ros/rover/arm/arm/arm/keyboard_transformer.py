#!/usr/bin/env python
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

ROS service which when queried with a key, 
    returns its position on the keyboard.
Used for auto typing task at URC
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: keyboard_mapper
SERVICES: get_key_position
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	arm
AUTHOR(S):  Anthony Lew
CREATION:	6/04/2024
EDITED:     6/04/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Properly implement auto transform
 - Verify the node works
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from geometry_msgs.msg import PointStamped
from arm_interfaces.srv import KeyPosition

import rclpy
from rclpy.node import Node

from tf2_ros import Buffer, TransformListener

import numpy as np

# hard coded coords (relative to center of keyboard in mm) 
# left = -x, right = +x, up = +y, down = -y 
# "key": (x, y) 
KEY_MAP = { 
    "esc": (-164, 50), "f1": (-126, 50), "f2": (-107, 50), "f3": (-88, 50), "f4": (-69, 50), "f5": (-40, 50), "f6": (-21, 50), "f7": (-2, 50), "f8": (17, 50), "f9": (46, 50), "f10": (65, 50), "f11": (84, 50), "f12": (103, 50), "prtsc": (126, 50), "scrlk": (145, 50), "pause": (164, 50),
    "`": (-164, 28), "1": (-145, 28), "2": (-126, 28), "3": (-107, 28), "4": (-88, 28), "5": (-69, 28), "6": (-50, 28), "7": (-31, 28), "8": (-12, 28), "9": (7, 28), "0": (26, 28), "-": (45, 28), "=": (64, 28), "backspace": (93, 28), "ins": (126, 28), "home": (145, 28), "pgup": (164, 28),
    "tab": (-159, 9), "q": (-135, 9), "w": (-116, 9), "e": (-97, 9), "r": (-78, 9), "t": (-59, 9), "y": (-40, 9), "u": (-21, 9), "i": (-2, 9), "o": (17, 9), "p": (36, 9), "[": (55, 9), "]": (74, 9), "\\": (97, 9), "del": (126, 9), "end": (145, 9), "pgdn": (164, 9),
    "capslock": (-157, -10), "a": (-130, -10), "s": (-111, -10), "d": (-92, -10), "f": (-73, -10), "g": (-54, -10), "h": (-35, -10), "j": (-16, -10), "k": (3, -10), "l": (22, -10), ";": (41, -10), "'": (60, -10), "enter": (91, -10),
    "lshift": (-152, -29), "z": (-121, -29), "x": (-102, -29), "c": (-83, -29), "v": (-64, -29), "b": (-45, -29), "n": (-26, -29), "m": (-7, -29), ",": (12, -29), ".": (31, -29), "/": (50, -29), "rshift": (86, -29), "uarrow": (145, -29),
    "lctrl": (-161, -48), "win": (-138, -48), "lalt": (-114, -48), "space": (-44, -48), "ralt": (29, -48), "fn": (53, -48), "menu": (76, -48), "rctrl": (101, -48), "larrow": (126, -48), "darrow": (145, -48), "rarrow": (164, -48) 
}

"""
HOW TO TEST:
nix-shell -p 'with import /home/nova/nova/nixfiles { }; pkgs.ros.nova-workspace.override {
	novaPackages = {
		inherit (pkgs.ros)
		nova-arm
		nova-arm-interfaces;
	};
}'
ros2 run arm keyboard_transformer.py

ros2 service call /get_key_position arm_interfaces/srv/KeyPosition '{key: "a"}'
"""

class KeyboardMapper(Node):
    def __init__(self):
        super().__init__('keyboard_mapper')

        self.service_name = self.declare_parameter('service_name', 'get_key_position').get_parameter_value().string_value
        self.keyboard_frame = self.declare_parameter('keyboard_frame', 'keyboard_frame').get_parameter_value().string_value
        self.base_frame = self.declare_parameter('base_frame', 'base_link').get_parameter_value().string_value

        self.srv = self.create_service(KeyPosition, self.service_name, self.get_key_position_callback)
        self.key_map = KEY_MAP

        # auto transform to base_link
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)

        self.get_logger().info(f"Running this service at {self.service_name}, looking for {self.keyboard_frame} to {self.base_frame}")

    def get_key_position_callback(self, request, response):
        if request.key.lower() not in self.key_map:
            self.get_logger().warn(f"Key {request.key} not found in map.")
            return response

        x, y = self.key_map[request.key.lower()]
        pos = PointStamped()
        pos.header.frame_id = self.keyboard_frame # can add support for multiple keyboards by changing response depending on request header frame
        #pos.header.stamp = request.header.stamp
        pos.point.x = x * 0.001 # convert mm to meters
        pos.point.y = y * 0.001
        pos.point.z = 0
        # attempt to transform position to base_link
        #transformed_pos = self.get_transform_to_base_link(pos)
        #if transformed_pos is None:
        #    self.get_logger().warn(f"Keyboard {self.keyboard_frame} transform could not be found.")
        #    response.position = pos 
        #    return response
        #response.position = transformed_pos 
        response.position = pos 
        self.get_logger().info(f"Returning request for {request.key}")
        return response

    def get_transform_to_base_link(self, key_pos):
        """ 
        Auto transform functionality to automatically transform key position 
        from relative to keyboard frame to relative to base link 
        so that position can be fed directly to path planner/ik
        """
        try:
            base_to_keyboard = self.tf_buffer.lookup_transform(self.base_frame, self.keyboard_frame, key_pos.header.stamp)
            transformed_point = tf2_geometry_msgs.do_transform_point(key_pos, base_to_keyboard)
        except:
            return None


def main():
    rclpy.init()
    node = KeyboardMapper()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()