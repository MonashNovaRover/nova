#!/usr/bin/env python3
from logging import Logger
from struct import pack
import jcan
from python_control.controllers.Card import Card
from python_control.controllers.Controller import Controller
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl


class CMDVelocityController(Controller):
    """Class to control the CMD card on the CAN bus"""
    def __init__(self, bus: jcan.Bus, logger: Logger, frame_id: hex, control: OneAxisVelocityControl):
        super().__init__(card=Card.CMD, max_value=32767, frame_id=frame_id, control=control, bus=bus, logger=logger)

    def get_frame(self) -> jcan.Frame:
        """Get the frame to send over the CAN bus"""
        control: OneAxisVelocityControl = self.get_control()

        # Set the data based on the direction, velocity, max value, and max percent
        data = int(control.get_direction().value * control.get_velocity() * self.get_max_value() * control.get_max_percent())

        # Check if the data is greater than the max value
        # If it is, set the data to the max value
        if data > self.get_max_value() or data < -self.get_max_value():
            data = self.get_max_value()

        # Pack the data into a list
        packed_data = list(pack('>h', int(data)))

        # Create and return the frame
        frame = jcan.Frame(id=self.frame_id, data=packed_data)

        return frame