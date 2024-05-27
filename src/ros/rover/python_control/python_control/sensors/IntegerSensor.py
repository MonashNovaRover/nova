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
            command_id: hex = None,
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
        self.command_id = command_id
    
    def callback_function(self, frame: jcan.Frame):
        """Update the sensor value based on the frame"""

        # Check if the frame data is the correct length
        if (self.command_id is None and len(frame.data) != 2) or \
            (self.command_id is not None and len(frame.data) != 3):
            return
        
        # Check if the command id is specified and is correct
        if self.command_id is not None and frame.data[0] != self.command_id:
            return
            
        # Set the sensor value
        signed_int = int.from_bytes([frame.data[-2], frame.data[-1]], byteorder='big', signed=True)
        self.set_sensor_value(signed_int)

    def publish_sensor(self):
        pass