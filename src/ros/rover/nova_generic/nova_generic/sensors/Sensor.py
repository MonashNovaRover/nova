#!/usr/bin/env python3

import abc
import jcan, logging
from typing import TypeVar, Generic
from nova_generic.sensors.sensor_parameters import sensor_parameters
from rclpy.node import Node

T = TypeVar('T')

class Sensor(Node, abc.ABC, Generic[T]):
    """Class to represent a sensor that publishes to a topic"""

    def __init__(self, name: str):
        super().__init__(name)

        # declare parameters
        self.sensor_param_listener = sensor_parameters.ParamListener(self)
        self.sensor_params = self.sensor_param_listener.get_params()

        # update logging level
        self.get_logger().setLevel(logging.getLevelNamesMapping()[self.sensor_params.logging_level])

        # start timers
        self.publish_timer = self.create_timer(self.sensor_params.publish_period, self.publish)

        # start up can bus
        self.bus = jcan.Bus()
        self.timer_jcan_spin = self.create_timer(1 / self.sensor_params.update_rate, self.bus.spin)
        self.bus.add_callback(self.sensor_params.frame_id, self.frame_callback)

        # signal successful start
        self.get_logger().info(f"{self.get_name()} listening to {self.sensor_params.frame_id}")

    def frame_callback(self, frame: jcan.Frame):
        """Update the sensor value based on the frame"""
        self.get_logger().debug(f"Received frame: {frame}")
        if frame.id != self.sensor_params.frame_id:
            self.get_logger().warn(f"Invalid frame id: {frame.id} != {self.sensor_params.frame_id}")
            return

        self.callback_function(frame)

    @abc.abstractmethod
    def publish(self):
        pass

    @abc.abstractmethod
    def callback_function(self, frame: jcan.Frame):
        pass