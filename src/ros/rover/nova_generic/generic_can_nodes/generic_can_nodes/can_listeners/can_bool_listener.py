#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS Node for listening to CAN bus messages,
converting them to booleans and publishing them to a topic
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        generic_can_nodes
AUTHOR:         Felicity Matthews
CREATION:	    01/07/2025
EDITED:		    01/07/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
to run node as an integer sensor with example config:
ros2 run generic_can_nodes CANBoolListener.py --ros-args -r __node:=bool_sensor --params-file /home/nova/nova/src/ros/rover/nova_generic/generic_can_nodes/generic_can_nodes/can_listeners/config/example.yaml
"""

import rclpy
import jcan
from generic_can_nodes.can_listeners.can_listener import CANListener
from generic_can_nodes.can_bool_listener_parameters import can_bool_listener_parameters
from generic_interfaces.msg import Bool

class CANBoolListener(CANListener):
    """Class to represent a CAN Listener that processes booleans"""

    def __init__(self, name: str = "CANBoolListener", message_type=Bool):
        super().__init__(name=name, message_type=message_type)

        # declare parameters
        self.param_listener = can_bool_listener_parameters.ParamListener(self)
        self.params = self.param_listener.get_params()

        assert not (self.params.true_msg < 0 and self.params.false_msg < 0), f"{self.get_name()} at least one of parameters true_msg ({self.params.true_msg}) and false_msg ({self.params.false_msg}) must be > 0"

    def create_msg(self, frame: jcan.Frame) -> Bool:
        """ Converts frame to boolean """
        msg = Bool()
        msg.header.stamp = self.get_clock().now().to_msg()

        if frame is None:
            msg.data = self.params.initial_value
            return msg

        state = frame.data[self.params.data_pos]

        if state == self.params.true_msg:
            msg.data = True
        elif state == self.params.false_msg:
            msg.data = False
        elif self.params.false_msg < 0:
            msg.data = False
        elif self.params.true_msg < 0:
            msg.data = True
        else:
            self.get_logger().warn(f"Unknown frame data: {frame} does not match true_msg: {self.params.true_msg} or false_msg: {self.params.false_msg}")

        return msg

def main():
    rclpy.init()
    can_bool_listener = CANBoolListener()
    rclpy.spin(can_bool_listener)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
