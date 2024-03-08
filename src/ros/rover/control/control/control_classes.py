#!/usr/bin/env python3

from enum import Enum
from struct import pack
import jcan


class Card(Enum):
    CMD = "CMD"
    JONO = "JONO"

class Direction(Enum):
    POSITIVE = 1
    NEGATIVE = -1

class OneAxisControl:
    def __init__(self, direction: Direction = Direction.POSITIVE, velocity: float = 0.0, max_percent: float = 1.0):
        self.direction = direction
        self.velocity = velocity
        self.max_percent = max_percent

    def update_direction(self, direction: Direction):
        if (direction == Direction.POSITIVE or direction == Direction.NEGATIVE):
            self.direction = direction
        else:
            raise ValueError("Invalid direction")
        
    def update_velocity(self, velocity: float):
        if (0.0 <= velocity <= 1.0):
            self.velocity = velocity
        else:
            raise ValueError("Invalid velocity")
        
    def update_max_percent(self, max_percent: float):
        if (0.0 <= max_percent <= 1.0):
            self.max_percent = max_percent
        else:
            raise ValueError("Invalid max_percent")

    def get_velocity(self):
        return self.velocity
    
    def get_direction(self):
        return self.direction
    
    def get_max_percent(self):
        return self.max_percent
        
    def stop(self):
        self.velocity = 0.0
        
class OneAxisControlLimits(OneAxisControl):
    def __init__(self, direction: Direction = Direction.POSITIVE, velocity: float = 0.0, max_percent: float = 1.0, limit_pos: bool = False, limit_neg: bool = False):
        super().__init__(direction, velocity, max_percent)
        self.limit_pos = limit_pos
        self.limit_neg = limit_neg

    def update_limit_pos(self, limit_pos: bool):
        self.limit_pos = limit_pos

    def update_limit_neg(self, limit_neg: bool):
        self.limit_neg = limit_neg
    
    def check_limits_hit(self):
        if ((self.direction == Direction.POSITIVE and self.limit_pos) or 
            (self.direction == Direction.NEGATIVE and self.limit_neg)):
            self.stop()
            return True
        else:
            return False

    def update_velocity(self, velocity: float, ignore_limits: bool = False):
        if ignore_limits or not self.check_limits_hit():
            super().update_velocity(velocity)

    def get_velocity(self):
        if self.check_limits_hit():
            return 0.0
        else:
            return self.velocity


class CardInterface():
    def __init__(self, card: Card, max_value: int, card_id: hex, control: OneAxisControl):
        self.card = card
        self.max_value = max_value
        self.card_id = card_id
        self.control = control

    def update_direction(self, direction: Direction):
        self.control.update_direction(direction)

    def update_velocity(self, velocity: float):
        self.control.update_velocity(velocity)

    def update_max_percent(self, max_percent: float):
        self.control.update_max_percent(max_percent)

    def update_velocity(self, velocity: float):
        if (velocity > 1.0):
            velocity = 1.0
        elif (velocity < 0.0):
            velocity = 0.0
        self.control.velocity = velocity

    def get_card(self):
        return self.card
    
    def get_max_value(self):
        return self.max_value
    
    def get_frame(self) -> jcan.Frame:
        pass


    
class JonoCardController(CardInterface):
    def __init__(self, card_id: hex, pos_command: hex, neg_command: hex, control: OneAxisControl):
        super().__init__(card=Card.JONO, max_value=255, card_id=card_id, control=control)
        self.pos_command = pos_command
        self.neg_command = neg_command
        self.control = control

    def get_frame(self) -> jcan.Frame:

        command: hex
        if self.control.direction == Direction.POSITIVE:
            command = self.pos_command
        else:
            command = self.neg_command
        
        data = int(self.control.velocity * self.max_value * self.control.max_percent)

        if data > self.max_value:
            data = self.max_value

        packed_data = [command, data]

        frame = jcan.Frame(id=self.card_id, data=packed_data)

        return frame
    

class CMDCardController(CardInterface):
    def __init__(self, card_id: hex, control: OneAxisControl):
        super().__init__(card=Card.CMD, max_value=32767, card_id=card_id)
        self.control = control

    def get_frame(self) -> jcan.Frame:
        
        data = int(self.control.direction.value * self.control.velocity * self.max_value * self.control.max_percent)

        packed_data = list(pack('>h', int(data)))

        frame = jcan.Frame(id=self.card_id, data=packed_data)

        return frame
    

            
            
        


