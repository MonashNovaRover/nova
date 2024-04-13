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

class LimitSwitchValue(Enum):
    """Enum for the different limits of the motors"""
    HIT = 0xFF
    CLEAR = 0x00

from typing import TypeVar, Generic

T = TypeVar('T')

class Sensor(Generic[T]):
    """Class to represent a sensor"""
    def __init__(self, frame_id: hex, initial_value: T = None):
        self.frame_id = frame_id # type: hex
        self.sensor_value = initial_value # type: T

    def set_sensor_value(self, sensor_value: T):
        """Set the sensor value"""
        self.sensor_value = sensor_value

    def get_sensor_value(self) -> T:
        """Get the sensor value"""
        return self.sensor_value
    
    @abc.abstractmethod
    def publish_sensor(self):
        pass

    @abc.abstractmethod
    def frame_callback(self, frame: jcan.Frame):
        pass
    
class TOFSensor(Sensor[int]):
    """Class to represent a time of flight sensor"""
    def __init__(self, frame_id: hex):
        super().__init__(frame_id=frame_id)
    
    def frame_callback(self, frame: jcan.Frame):
        """Update the sensor value based on the frame"""
        if frame.id == self.frame_id:
            self.set_sensor_value(int(frame.data[1] + (frame.data[0] << 8)))

class LimitSwitchSensor(Sensor[bool]):
    """Class to represent a limit switch sensor"""
    def __init__(self, frame_id: hex, id: hex):
        super().__init__(frame_id=frame_id)
        self.id = id # type: hex

    def frame_callback(self, frame: jcan.Frame):
        """Update the sensor value based on the frame"""
        if frame.id == self.frame_id and frame.data[0] == self.id:
            self.set_sensor_value(frame.data[1] == LimitSwitchValue.HIT.value)

class Limit():
    """Class to represent a limit"""
    def __init__(self, frame_id: hex):
        self.frame_id = frame_id # type: hex
        self.limit_hit = False # type: bool

    def update_limit_hit(self, limit_hit: bool):
        """Update the limit hit"""
        self.limit_hit = limit_hit
    
    def get_limit_hit(self):
        """Get the limit hit"""
        return self.limit_hit

class LimitSwitch(Limit):
    """Class to represent a limit switch"""
    def __init__(self, frame_id: hex, id: hex):
        super().__init__(frame_id=frame_id)
        self.id = id # type: hex

    def frame_callback(self, frame: jcan.Frame):
        """Update the limit hit based on the frame"""
        if frame.id == self.frame_id and frame.data[0] == self.id:
            self.update_limit_hit(frame.data[1] == LimitSwitchValue.CLEAR.value)

class TOFLimit(Limit):
    """Class to represent a time of flight sensor"""
    def __init__(self, frame_id: hex, direction: Direction, max_limit: int = None, min_limit: int = None):
        super().__init__(frame_id=frame_id)
        self.direction = direction # type: Direction
        self.max_limit = max_limit # type: int
        self.min_limit = min_limit # type: int
        self.tof_value = 0 # type: int

    def set_tof_value(self, tof_value: int):
        """Set the time of flight value"""
        self.tof_value = tof_value

    def frame_callback(self, frame: jcan.Frame):
        """Update the limit hit based on the frame"""
        if frame.id != self.frame_id:
            return
        if len(frame.data) == 2:
            return
        
        raw_tof = int(frame.data[1] + (frame.data[0] << 8))



class OneAxisLimitsController():
    def __init__(self, pos_limit: Limit, neg_limit: Limit):
        self.pos_limit = pos_limit
        self.neg_limit = neg_limit


        


    
                


class OneAxisControl:
    """Class to control a single axis motor"""

    def __init__(self, direction: Direction = Direction.POSITIVE, velocity: float = 0.0, max_percent: float = 1.0):
        self.direction = direction # type: Direction
        self.velocity = velocity # type: float [0.0, 1.0]
        self.max_percent = max_percent # type: float [0.0, 1.0]
        self.pos_limit = None # type: bool
        self.neg_limit = None # type: bool

    def update_direction(self, direction: Direction):
        """Update the direction of the motor"""
        if (direction == Direction.POSITIVE or direction == Direction.NEGATIVE):
            self.direction = direction
        else:
            raise ValueError("Invalid direction")
        
    def update_velocity(self, velocity: float, ignore_limits: bool = False):
        """Updates the velocity of the motor, if ignore_limits is True, the limits are ignored"""
        if ignore_limits or not self.check_limits_hit():
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

    def update_limit_pos(self, limit_pos: bool):
        """Update the positive limit of the motor"""
        self.limit_pos = limit_pos

    def update_limit_neg(self, limit_neg: bool):
        """Update the negative limit of the motor"""
        self.limit_neg = limit_neg

    def has_limits(self):
        """Check if the motor has limits"""
        return self.limit_pos is not None and self.limit_neg is not None
    
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
    

            
            
        


