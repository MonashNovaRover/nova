#!/usr/bin/env python3

"""
to run node as an integer sensor with example config:
ros2 run nova_generic CANNumberListener.py --ros-args -r __node:=integer_sensor --params-file /home/nova/nova/src/ros/rover/nova_generic/nova_generic/can_listeners/example.yaml
"""

import rclpy
import jcan
from nova_generic.can_listeners.can_listeners.CANListener import CANListener
from nova_generic.can_number_listener_parameters import can_number_listener_parameters
from nova_interfaces.msg import NumberReading

class CANBoolListener(CANListener):
    """Class to represent a CAN Listener that processes booleans"""

    def __init__(self):
        super().__init__(name="CANBoolListener", message_type=NumberReading)

        # declare parameters
        self.param_listener = can_number_listener_parameters.ParamListener(self)
        self.params = self.param_listener.get_params()

    def create_msg(self, frame: jcan.Frame) -> NumberReading:
        """ Converts frame to integer and then applies scale and offset"""
        msg = NumberReading()
        msg.header.stamp = self.get_clock().now().to_msg()

        if frame is None:
            msg.data = self.params.initial_value
            return msg

        data = [frame.data[i] for i in range(-self.params.length, 0)]

        signed_int = int.from_bytes(data, byteorder='big', signed=True)
        msg.data = signed_int * self.params.scale + self.params.offset

        return msg

def main():
    rclpy.init()
    can_number_listener = CANNumberListener()
    rclpy.spin(can_number_listener)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
