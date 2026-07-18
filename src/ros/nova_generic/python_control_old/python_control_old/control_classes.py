#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Classes used by the control nodes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: None
TOPICS: None
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    control
AUTHOR(S):	Tristan Clark
CREATION:	08/03/2024
EDITED:		08/03/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""

import abc
from enum import Enum
from struct import pack
import jcan

class Card(Enum):
    """Enum for the different cards on the CAN bus"""
    CMD = "CMD"
    JONO = "JONO"

class Direction(Enum):
    """Enum for the different directions of the motors"""
    POSITIVE = 1
    NEGATIVE = -1

class OneAxisControl:
    """Class to control a single axis motor"""

    def __init__(self, direction: Direction = Direction.POSITIVE, velocity: float = 0.0, max_percent: float = 1.0):
        self.direction = direction # type: Direction
        self.velocity = velocity # type: float [0.0, 1.0]
        self.max_percent = max_percent # type: float [0.0, 1.0]

    def update_direction(self, direction: Direction):
        """Update the direction of the motor"""
        if (direction == Direction.POSITIVE or direction == Direction.NEGATIVE):
            self.direction = direction
        else:
            raise ValueError("Invalid direction")
        
    def update_velocity(self, velocity: float):
        """Update the velocity of the motor"""
        if (0.0 <= velocity <= 1.0):
            self.velocity = velocity
        else:
            raise ValueError("Invalid velocity")
        
    def update_max_percent(self, max_percent: float):
        """Update the max percent of the motor"""
        if (0.0 <= max_percent <= 1.0):
            self.max_percent = max_percent
        else:
            raise ValueError("Invalid max_percent")
        

    def get_velocity(self):
        """Get the velocity of the motor"""
        return self.velocity
    

    def get_direction(self):
        """Get the direction of the motor"""
        return self.direction
    

    def get_max_percent(self):
        """Get the max percent of the motor"""
        return self.max_percent
        
    def stop(self):
        """Stop the motor"""
        self.velocity = 0.0
        self.direction = Direction.POSITIVE
        
class OneAxisControlLimits(OneAxisControl):
    """Class to control a single axis motor with limits"""
    def __init__(self, direction: Direction = Direction.POSITIVE, velocity: float = 0.0, max_percent: float = 1.0, limit_pos: bool = False, limit_neg: bool = False):
        super().__init__(direction, velocity, max_percent)
        self.limit_pos = limit_pos # type: bool
        self.limit_neg = limit_neg # type: bool

    def update_limit_pos(self, limit_pos: bool):
        """Update the positive limit of the motor"""
        self.limit_pos = limit_pos

    def update_limit_neg(self, limit_neg: bool):
        """Update the negative limit of the motor"""
        self.limit_neg = limit_neg
    
    def check_limits_hit(self):
        """
        Check if the limits of the motor have been hit
        Stops the motor if the limits have been hit
        :return: bool
        """
        if ((self.direction == Direction.POSITIVE and self.limit_pos) or 
            (self.direction == Direction.NEGATIVE and self.limit_neg)):
            self.stop()
            return True
        else:
            return False

    def update_velocity(self, velocity: float, ignore_limits: bool = False):
        """Updates the velocity of the motor, if ignore_limits is True, the limits are ignored"""
        if ignore_limits or not self.check_limits_hit():
            super().update_velocity(velocity)


class CardInterface():
    """Class to interface with the cards on the CAN bus"""
    def __init__(self, card: Card, max_value: int, card_id: hex, control: OneAxisControl):
        self.card = card # type: Card
        self.max_value = max_value # type: int
        self.card_id = card_id # type: hex
        self.control = control # type: OneAxisControl

    def get_card(self):
        """Get the card type"""
        return self.card

    def get_max_value(self):
        """Get the max value of the card"""
        return self.max_value
    
    @abc.abstractmethod
    def get_frame(self) -> jcan.Frame:
        pass


    
class JonoCardController(CardInterface):
    """Class to control the JONO card on the CAN bus"""
    def __init__(self, card_id: hex, pos_command: hex, neg_command: hex, control: OneAxisControl):
        super().__init__(card=Card.JONO, max_value=255, card_id=card_id, control=control)
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
    

class CMDCardController(CardInterface):
    """Class to control the CMD card on the CAN bus"""
    def __init__(self, card_id: hex, control: OneAxisControl):
        super().__init__(card=Card.CMD, max_value=32767, card_id=card_id, control=control)

    def get_frame(self) -> jcan.Frame:
        """Get the frame to send over the CAN bus"""

        # Set the data based on the direction, velocity, max value, and max percent
        data = int(self.control.get_direction().value * self.control.get_velocity() * self.get_max_value() * self.control.get_max_percent())

        # Check if the data is greater than the max value
        # If it is, set the data to the max value
        if data > self.get_max_value():
            data = self.get_max_value()

        # Pack the data into a list
        packed_data = list(pack('>h', int(data)))

        # Create and return the frame
        frame = jcan.Frame(id=self.card_id, data=packed_data)

        return frame