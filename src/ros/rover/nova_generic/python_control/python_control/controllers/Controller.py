#!/usr/bin/env python3
import abc
from logging import Logger
import jcan
from python_control.controllers.Card import Card
from python_control.controls.Control import Control

class Controller(abc.ABC):
    """Class to interface with the cards on the CAN bus"""
    def __init__(self, card: Card, max_value: int, frame_id: hex, control: Control, bus: jcan.Bus, logger: Logger, send_continuously: bool = True):
        self.card = card # type: Card
        self.max_value = max_value # type: int
        # Card Id = 12 bits, (0x000 - 0xFFF)
        self.frame_id = frame_id # type: hex
        self.control = control # type: Control
        self.bus = bus # type: jcan.Bus
        self.logger = logger
        self.send_continuously = send_continuously # type: bool
        self.last_frame = jcan.Frame(000, [0000]) # type: jcan.Frame

    def get_logger(self) -> Logger:
        return self.logger
    
    def get_card(self) -> Card:
        """Get the card type"""
        return self.card
    
    def get_control(self) -> Control:
        """"""
        return self.control

    def get_max_value(self) -> int:
        """Get the max value of the card"""
        return self.max_value
    
    def control_send_callback(self) -> None:
        frame = self.get_frame()
        if frame is None:
            return

        # only send can command when they change if toggled
        if not self.send_continuously and self.last_frame.data == frame.data and self.last_frame.id == frame.id:
            return

        self.last_frame = frame
        self.get_logger().debug(f"Sending frame: {frame}")
        self.bus.send(frame)

    def stop(self) -> None:
        """Stop the controller"""
        self.control.stop()
    
    @abc.abstractmethod
    def get_frame(self) -> jcan.Frame:
        pass

