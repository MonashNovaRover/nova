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
 - Find an IRL alignment
 - Make a rectangle overlay on camera in GUI
 - Make the OPENCV auto aligner (for more complex solution)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from geometry_msgs.msg import PointStamped, TransformStamped
from arm_interfaces.srv import KeyPosition, StringTrigger

import rclpy
from rclpy.node import Node

from tf2_ros import Buffer, TransformListener, TransformBroadcaster

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

DEFAULT_POSITION = [0.0, 0.0, 0.0]
DEFAULT_QUATERNION = [0.0, 0.0, 0.0, 1.0]

"""
HOW TO TEST:
nix-shell -p 'with import /home/nova/nova/nixfiles { }; pkgs.ros.nova-workspace.override {
	novaPackages = {
		inherit (pkgs.ros)
		nova-arm
		nova-arm-interfaces;
	};
}'
ros2 run arm keyboard_localiser.py

ros2 service call /get_key_position arm_interfaces/srv/KeyPosition '{key: "a"}'

In separate terminal:
- Run GUI
- Navigate to urc/auto-typing
"""

KEY_SERVICE_NAME = '/arm/keyboard/get_key_position'
KEYBOARD_TF_SERVICE_NAME = '/arm/keyboard/transform_toggle'


class KeyboardLocaliser(Node):
    def __init__(self):
        super().__init__('keyboard_localiser')

        # key position initalisation
        self.keyboard_frame = self.declare_parameter('keyboard_frame', 'keyboard_frame').get_parameter_value().string_value
        self.base_frame = self.declare_parameter('base_frame', 'base_link').get_parameter_value().string_value
        self.key_srv = self.create_service(KeyPosition, KEY_SERVICE_NAME, self.get_key_position_callback)
        self.key_map = KEY_MAP

        # keyboard alignment initalisation
        self.aligned_keyboard_position = self.declare_parameter('aligned_keyboard_position', DEFAULT_POSITION).get_parameter_value().double_array_value
        self.aligned_keyboard_quaternion = self.declare_parameter('aligned_keyboard_quaternion', DEFAULT_QUATERNION).get_parameter_value().double_array_value
        self.align_srv = self.create_service(StringTrigger, KEYBOARD_TF_SERVICE_NAME, self.get_align_callback)
        self.is_aligned = False

        # tf2 initalisation
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)
        self.transform_broadcaster = TransformBroadcaster(self)
        
        # timer to constantly publish tf at period/second
        timer_period = self.declare_parameter('tf_publish_rate', 1.0).get_parameter_value().double_value
        self.create_timer(timer_period, self.publish_tf)

        self.get_logger().info(f"Running this node with services: {KEY_SERVICE_NAME}, {KEYBOARD_TF_SERVICE_NAME}. Using keyboard: {self.keyboard_frame} for transforms and base link: {self.base_frame}")

    def get_key_position_callback(self, request, response):
        if request.key.lower() not in self.key_map:
            self.get_logger().warn(f"Key {request.key} not found in map.")
            return response

        x, y = self.key_map[request.key.lower()]
        pos = PointStamped()
        pos.header.frame_id = self.keyboard_frame # can add support for multiple keyboards by changing response depending on request header frame
        pos.header.stamp = request.header.stamp
        pos.point.x = x * 0.001 # convert mm to meters
        pos.point.y = y * 0.001
        pos.point.z = 0
        
        # attempt to transform position to base_link
        transformed = self.get_transform_to_base_link(pos)
        if transformed is None:
            self.get_logger().warn(f"Keyboard {self.keyboard_frame} transform could not be found.")
            return response

        response.position = transformed 
        self.get_logger().info(f"Returning request for {request.key}")
        return response

    def get_align_callback(self, request, response):
        """Once aligned, begin publishing TF"""
        self.get_logger().info(f"Toggling alignment for {request.value}")
        if request.value == "start":
            self.is_aligned = True
            response.message = f"Aligned. TF has begun publishing under {self.keyboard_frame} relative to {self.base_frame}"
        elif request.value == "stop":
            self.is_aligned = False
            response.message = f"TF publishing has been stopped"
        else:
            response.success = self.is_aligned
            response.message = f"\"{request.value}\" not recognised as a valid value."
            return response

        response.success = self.is_aligned
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

    def publish_tf(self) -> None:
        '''Publish the transform of the keyboard'''
        if not self.is_aligned:
            return
        
        tfs = TransformStamped()
        tfs.header.stamp = self.get_clock().now().to_msg()
        tfs.header.frame_id = self.base_frame
        tfs.child_frame_id = self.keyboard_frame
        tfs.transform.translation.x, tfs.transform.translation.y, tfs.transform.translation.z, = self.aligned_keyboard_position
        tfs.transform.rotation.x, tfs.transform.rotation.y, tfs.transform.rotation.z, tfs.transform.rotation.w = self.aligned_keyboard_quaternion

        self.transform_broadcaster.sendTransform(tfs)


def main():
    rclpy.init()
    node = KeyboardLocaliser()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()