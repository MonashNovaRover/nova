#!/usr/bin/env python3

from python_control.sensors.Sensor import Sensor
from logging import Logger
import jcan
from enum import Enum

class LimitSwitchValue(Enum):
    """Enum for the different limits of the motors"""
    HIT = 0xFF
    CLEAR = 0x00

class LimitSwitchSensor(Sensor[bool]):
    """Class to represent a limit switch sensor"""
    def __init__(
            self, 
            bus: jcan.Bus, 
            logger: Logger, 
            frame_id: hex, 
            command_id: hex, 
            initial_value: bool = False,
            run_can: bool = True
        ):
        super().__init__(
            bus=bus, 
            logger=logger, 
            frame_id=frame_id, 
            initial_value=initial_value,
            run_can=run_can
        )
        # Command ID = 1 Byte, (0x00 - 0xFF)
        self.command_id = command_id # type: hex

    def callback_function(self, frame: jcan.Frame):
        """Update the sensor value based on the frame"""
        if frame.data[0] != self.command_id:
            raise ValueError("Invalid command id")
        
        self.set_sensor_value(frame.data[1] == LimitSwitchValue.HIT.value)
    
    def publish_sensor(self):
        pass