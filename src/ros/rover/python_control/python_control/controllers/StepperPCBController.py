#!/usr/bin/env python3
from logging import Logger
from struct import pack
import jcan
from python_control.controllers.Card import Card
from python_control.controllers.Controller import Controller
from python_control.controls.OneAxisPositionControl import OneAxisPositionControl
from python_control.sensors.CommandSensor import CommandSensor

class StepperPCBPositionController(Controller):
    """Class to control the CMD card on the CAN bus"""
    def __init__(self, bus: jcan.Bus, logger: Logger, frame_id: hex, pos_command_id: hex, zero_command_id: hex, control: OneAxisPositionControl, zero_sensor: CommandSensor):
        super().__init__(card=Card.STEPPER_PCB, max_value=32767, frame_id=frame_id, control=control, bus=bus, logger=logger)
        # Command Id = 1 Byte, (0x00 - 0xFF)
        self.pos_command_id = pos_command_id # type: hex
        self.zero_command_id = zero_command_id # type: hex
        self.stopped = False
        self.zeroing = False
        self.zero_sensor = zero_sensor


    def get_frame(self) -> jcan.Frame:
        """Get the frame to send over the CAN bus"""
        control: OneAxisPositionControl = self.get_control()

        # Set the data based on the position
        data = int(control.get_goal_position())

        # Check if the data is greater than the max value
        # If it is, set the data to the max value
        if data > self.get_max_value() or data < -self.get_max_value():
            data = self.get_max_value()

        # Pack the data into a list
        packed_data = [int(self.pos_command_id)] + list(pack('>h', int(data)))

        # Create and return the frame
        frame = jcan.Frame(id=self.frame_id, data=packed_data)

        return frame
    
    def stop(self):
        """Stop the controller"""
        super().stop()
        self.stopped = True

    def start(self):
        """Start the controller"""
        self.stopped = False

    def zero(self):
        self.stop()
        self.zeroing = True
        self.get_logger().info("Start zeroing")
        self.control.zero()
        frame = jcan.Frame(id=self.frame_id, data=[int(self.zero_command_id)])
        self.get_logger().debug(f"Sending frame: {frame}")
        self.bus.send(frame)

    def go_to_position(self, name: str):
        self.start()
        self.control.go_to_position(name)

    def is_zeroing(self):
        return self.zeroing
        
    def control_send_callback(self):
        if self.zeroing:
            if self.zero_sensor.get_sensor_value():
                self.zeroing = False
                self.start()
                self.zero_sensor.reset()
                self.get_logger().debug("Zeroed")
            else:
                self.get_logger().debug("Zeroing")
        elif not self.stopped:
            super().control_send_callback()
        else:
            self.get_logger().debug("Controller is stopped, not sending frame")
        
        