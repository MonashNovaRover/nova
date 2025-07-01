#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS Node for listening to CAN bus messages,
and publishing them to a topic
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        generic_can_nodes
AUTHOR:         Felicity Matthews
CREATION:	    01/07/2025
EDITED:		    01/07/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import abc
import jcan, logging
from generic_can_nodes.can_listener_parameters import can_listener_parameters
from rclpy.node import Node

class CANListener(Node, abc.ABC):
    """Class to represent a sensor that publishes to a topic"""

    def __init__(self, name: str, message_type):
        super().__init__(name)

        # declare parameters
        self.can_listener_param_listener = can_listener_parameters.ParamListener(self)
        self.listener_params = self.can_listener_param_listener.get_params()

        # define variables
        self.last_frame = None

        # update logging level
        self.get_logger().set_level(logging.getLevelNamesMapping()[self.listener_params.logging_level])

        # start timers
        self.publish_timer = self.create_timer(self.listener_params.publish_period, self.publish)

        # create publisher
        self.publisher = self.create_publisher(message_type, self.listener_params.topic, 10)

        # start up can bus
        self.bus = jcan.Bus()
        self.timer_jcan_spin = self.create_timer(1 / self.listener_params.update_rate, self.bus.spin)
        self.bus.add_callback(self.listener_params.frame_id, self.frame_callback)
        self.bus.open(self.listener_params.can_bus)

        # signal successful start
        self.get_logger().info(f"{self.get_name()} listening to {self.listener_params.frame_id}")

    def frame_callback(self, frame: jcan.Frame):
        """Records the frame received from the CAN bus"""
        self.get_logger().debug(f"Received frame: {frame}")
        if frame.id != self.listener_params.frame_id:
            self.get_logger().warn(f"Invalid frame id: {frame.id} != {self.listener_params.frame_id}")
            return

        if 0 < self.listener_params.command_id != frame.data[0]:
            return

        self.last_frame = frame

    def publish(self):
        """Creates a msg and then publishes it"""
        msg = self.create_msg(self.last_frame)

        if type(msg) != self.publisher.msg_type:
            self.get_logger().warn(f"mismatched msg type, expecting {self.publisher.msg_type} but got {msg} of type {type(msg)}")
            return

        self.publisher.publish(msg)

    @abc.abstractmethod
    def create_msg(self, frame: jcan.Frame):
        pass
