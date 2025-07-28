#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS Node for listening to CAN bus messages,
converting them to numbers and publishing them to a topic
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        generic_can_nodes
AUTHOR:         Felicity Matthews
CREATION:	    01/07/2025
EDITED:		    01/07/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
to run node as an integer sensor with example config:
ros2 run generic_can_nodes CANNumberListener.py --ros-args -r __node:=integer_sensor --params-file /home/nova/nova/src/ros/rover/nova_generic/generic_can_nodes/generic_can_nodes/can_listeners/config/example.yaml
"""

import rclpy
import jcan
from generic_can_nodes.can_listeners.can_listener import CANListener
from generic_can_nodes.can_number_listener_parameters import can_number_listener_parameters
from generic_interfaces.msg import Float64

class CANNumberListener(CANListener):
    """Class to represent a CAN Listener that processes numbers"""

    def __init__(self, name: str = "CANNumberListener", message_type=Float64):
        super().__init__(name=name, message_type=message_type)

        # declare parameters
        self.param_listener = can_number_listener_parameters.ParamListener(self)
        self.params = self.param_listener.get_params()

    def mapper(self, num: float) -> float:
        """ Applies a scale and offset to the number """
        return num * self.params.scale + self.params.offset

    def create_msg(self, frame: jcan.Frame) -> Float64:
        """ Converts frame to an integer and then mapper function to the data """
        msg = Float64()
        msg.header.stamp = self.get_clock().now().to_msg()

        if frame is None:
            msg.data = self.params.initial_value
            return msg

        # gets the last length number of frames - excludes the command id if there is one.
        start_pos = 0 if self.listener_params.command_id < 0 else 1
        end_pos = len(frame.data) if self.params.length < 0 else start_pos + self.params.length
        data = [frame.data[i] for i in range(start_pos, end_pos)]

        signed_int = int.from_bytes(data, byteorder='big', signed=True)
        msg.data = self.mapper(signed_int)

        return msg

def main():
    rclpy.init()
    can_number_listener = CANNumberListener()
    rclpy.spin(can_number_listener)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
