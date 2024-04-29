#!/usr/bin/env python3

from logging import Logger
import jcan
from python_control.classes.cards.Card import Card
from python_control.classes.cards.CardController import CardController
from python_control.classes.controls.OneAxisControl import OneAxisControl
from python_control.classes.controls.OneAxisControl import Direction

class JonoCardController(CardController):
    """Class to control the JONO card on the CAN bus"""
    def __init__(self, card_id: hex, pos_command: hex, neg_command: hex, control: OneAxisControl, bus: jcan.Bus, logger: Logger):
        super().__init__(card=Card.JONO, max_value=255, card_id=card_id, control=control, bus=bus, logger=logger)
        self.pos_command = pos_command # type: hex
        self.neg_command = neg_command # type: hex

    def get_frame(self) -> jcan.Frame:
        """Get the frame to send over the CAN bus"""

        # Set the command based on the direction
        command: hex
        if self.control.get_direction() == Direction.POSITIVE:
            command = self.pos_command
        else:
            command = self.neg_command
        
        # Set the data based on the velocity, max value, and max percent
        data = int(self.control.get_velocity() * self.get_max_value() * self.control.get_max_percent())

        # Check if the data is greater than the max value
        # If it is, set the data to the max value
        if data > self.get_max_value():
            data = self.get_max_value()

        # Pack the data into a list
        packed_data = [command, data]

        # Create and return the frame
        frame = jcan.Frame(id=self.card_id, data=packed_data)

        return frame