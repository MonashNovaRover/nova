#!/usr/bin/env python3

from logging import Logger
import jcan
from python_control.sensors.Sensor import Sensor
from rclpy.publisher import Publisher

class IntegerSensor(Sensor[int]):
    """Class to represent a time of flight sensor"""
    def __init__(
            self, 
            bus: jcan.Bus, 
            logger: Logger, 
            frame_id: hex, 
            publisher: Publisher = None, 
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
    
    def callback_function(self, frame: jcan.Frame):
        """Update the sensor value based on the frame"""
        if len(frame.data) != 2:
            raise ValueError("Invalid frame data")
        
        self.set_sensor_value(int(frame.data[1] + (frame.data[0] << 8)))

    def publish_sensor(self):
        pass