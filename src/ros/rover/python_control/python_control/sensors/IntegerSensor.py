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
        if self.command_id is None and len(frame.data) != 2:
            self.logger.error("Invalid frame data")
            return
        elif self.command_id is not None and len(frame.data) != 3:
            self.logger.error("Invalid frame data")
            return
        
        # Check if the command id is specified
        if self.command_id is not None:
            # Check if the command id is correct
            if frame.data[0] != self.command_id:
                self.logger.error("Invalid command id")
                return
            # Set the sensor value
            self.set_sensor_value(int(frame.data[2] + (frame.data[1] << 8)))
        else:
            # Set the sensor value
            self.set_sensor_value(int(frame.data[1] + (frame.data[0] << 8)))

    def get_sensor_value(self) -> int:
        """Get the sensor value"""
        return super().get_sensor_value()

    def publish_sensor(self):
        pass