#!/usr/bin/env python3


import rclpy
import jcan
from nova_generic.can_listeners.CANListener import CANListener
from nova_generic.can_bool_listener_parameters import can_bool_listener_parameters
from generic_interfaces.Bool.msg import Bool

class CANBoolListener(CANListener):
    """Class to represent a CAN Listener that processes booleans"""

    def __init__(self):
        super().__init__(name="CANBoolListener", message_type=Bool)

        # declare parameters
        self.param_listener = can_bool_listener_parameters.ParamListener(self)
        self.params = self.param_listener.get_params()

        assert not (self.params.true_msg < 0 and self.params.false_msg < 0), f"{self.get_name()} at least one of parameters true_msg and false_msg must be > 0"

    def create_msg(self, frame: jcan.Frame) -> Bool:
        """ Converts frame to boolean """
        msg = Bool()
        msg.header.stamp = self.get_clock().now().to_msg()

        if frame is None:
            msg.data = self.params.initial_value
            return msg

        state = frame.data[self.params.data_pos]

        if state == self.true_msg:
            msg.data = True
        elif state == self.false_msg:
            msg.data = False
        elif self.false_msg < 0:
            msg.data = False
        elif self.true_msg < 0:
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
