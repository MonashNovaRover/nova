#!/usr/bin/env python3

from logging import Logger
import jcan
from python_control.controllers.Card import Card
from python_control.controllers.Controller import Controller
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl
from python_control.controls.Direction import Direction

class JonoVelocityController(Controller):
    """Class to control the JONO card on the CAN bus"""
    def __init__(self, frame_id: hex, pos_command: hex, neg_command: hex, control: OneAxisVelocityControl, bus: jcan.Bus, logger: Logger):
        super().__init__(card=Card.JONO, max_value=255, frame_id=frame_id, control=control, bus=bus, logger=logger)
        # Command Ids = 1 Byte, (0x00 - 0xFF)
        self.pos_command = pos_command # type: hex
        self.neg_command = neg_command # type: hex

    def get_frame(self) -> jcan.Frame:
        """Get the frame to send over the CAN bus"""
        control: OneAxisVelocityControl = self.get_control()

        # Set the command based on the direction
        command: hex
        if control.get_direction() == Direction.POSITIVE:
            command = self.pos_command
        else:
            command = self.neg_command
        
        # Set the data based on the velocity, max value, and max percent
        data = int(control.get_velocity() * self.get_max_value() * control.get_max_percent())

        # Check if the data is greater than the max value
        # If it is, set the data to the max value
        if data > self.get_max_value():
            data = self.get_max_value()

        # Pack the data into a list
        packed_data = [command, data]

        # Create and return the frame
        frame = jcan.Frame(id=self.frame_id, data=packed_data)

        return frame