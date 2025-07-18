#!/usr/bin/env python3

from logging import Logger
import jcan
from python_control.sensors.IntegerSensor import IntegerSensor
from sensor_msgs.msg import Range
from rclpy.publisher import Publisher

class RangeSensor(IntegerSensor):
    """
    Class to represent a Range based sensor
    Examples include: Time of Flight sensors (TOF)
    """
    def __init__(
            self, 
            bus: jcan.Bus, 
            logger: Logger, 
            frame_id: hex, 
            publisher: Publisher = None, 
            maximum: int = 1000, 
            minimum: int = 0, 
            offset: int = 0, 
            initial_value: int = 0,
            run_can: bool = True
        ):
        super().__init__(
            bus=bus, 
            logger=logger, 
            frame_id=frame_id, 
            publisher=publisher, 
            initial_value=initial_value, 
            run_can=run_can
        )
        self.maximum = maximum
        self.minimum = minimum
        self.offset = offset

    def set_sensor_value(self, sensor_value: int):
        """Set the sensor value"""
        self.sensor_value = sensor_value - self.offset

    def publish_sensor(self):
        """
        Publishes the value of the Range sensor as a Range message
        """
        self.get_logger().info(f"Publishing Range Sensor Value: {self.get_sensor_value()}")
        msg = Range()
        msg.range = float(self.get_sensor_value())
        msg.min_range = float(self.minimum)
        msg.max_range = float(self.maximum)
        self.publisher.publish(msg)