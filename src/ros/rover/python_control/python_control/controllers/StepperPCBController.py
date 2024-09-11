#!/usr/bin/env python3
from logging import Logger
from struct import pack
import jcan
from python_control.controllers.StepperPositionController import StepperPositionController
from python_control.controllers.Card import Card
from python_control.controls.OneAxisPositionControl import OneAxisPositionControl

class StepperPCBPositionController(StepperPositionController):
    """Class to control the CMD card on the CAN bus"""
    def __init__(
            self,
            bus: jcan.Bus, 
            logger: Logger, 
            frame_id: hex, 
            control: OneAxisPositionControl,
            pos_command_id: hex, 
            zero_command_id: hex = None, 
            set_command_id: hex = None,           
        ):
        super().__init__(
            card=Card.STEPPER_PCB, 
            max_value=32767, 
            frame_id=frame_id, 
            control=control, 
            bus=bus, 
            logger=logger,
        )
        # Command Id = 1 Byte, (0x00 - 0xFF)
        self.pos_command_id = pos_command_id # type: hex
        self.zero_command_id = zero_command_id # type: hex
        self.set_command_id = set_command_id # type: hex

    def get_frame(self) -> jcan.Frame:
        """Get the frame to send over the CAN bus"""
        control: OneAxisPositionControl = self.get_control()
        extra_data = []

        if ((self.is_zeroing() and self.zero_command_id is None) or \
            (self.is_setting() and self.set_command_id is None)) and \
            (not self.is_going_to_position()):
            return None

        if self.is_zeroing():
            command = self.zero_command_id
        else:
            if self.is_setting():
                command = self.set_command_id
            else:
                command = self.pos_command_id

            # Set the data based on the position
            data = int(control.get_goal_position())

            # Check if the data is greater than the max value
            # If it is, set the data to the max value
            if data > self.get_max_value() or data < -self.get_max_value():
                data = self.get_max_value()

            extra_data = list(pack('>h', int(data)))
                        

        # Pack the data into a list
        packed_data = [int(command)] + extra_data

        # Create and return the frame
        frame = jcan.Frame(id=self.frame_id, data=packed_data)

        return frame
        
  

 
        
        