#!/usr/bin/env python3

from logging import Logger
import jcan
from python_control.controllers.Card import Card
from python_control.controllers.Controller import Controller
from python_control.controls.ContinuousOneAxisPositionControl import ContinuousOneAxisPositionControl

class JonoPositionController(Controller):
    """Class to control the JONO card on the CAN bus"""
    def __init__(
            self,
            max_value: int,
            pos_command: int,
            bus: jcan.Bus,
            logger: Logger,
            frame_id: hex,
            control: ContinuousOneAxisPositionControl,
    ):
        super().__init__(
            card=Card.JONO,
            max_value=max_value,
            frame_id=frame_id,
            control=control,
            bus=bus,
            logger=logger)
        self.pos_command = pos_command

    def get_frame(self) -> jcan.Frame:
        """Get the frame to send over the CAN bus"""
        control: ContinuousOneAxisPositionControl = self.get_control()

        if control.get_target_position() is None:
            return

        # Set the command based on the direction
        command = self.pos_command

        # Set the data based on the velocity, max value, and max percent
        data = control.get_target_position() / control.get_max_angle() * self.get_max_value()

        # Check if the data is greater than the max value
        # If it is, set the data to the max value
        if data > self.get_max_value():
            data = self.get_max_value()

        # Pack the data into a list
        packed_data = [command, data]

        # Create and return the frame
        frame = jcan.Frame(id=self.frame_id, data=packed_data)

        return frame
