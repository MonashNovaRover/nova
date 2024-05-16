#!/usr/bin/env python3
from logging import Logger
from struct import pack
import jcan
from python_control.controllers.Card import Card
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl
from python_control.controllers import CMDVelocityController
from python_control.controls.TimedOneAxisVelocityControl import TimedOneAxisVelocityControl
import time


class TimedCMDVelocityController(CMDVelocityController):
    """Class to control the CMD card on the CAN bus"""
    def __init__(self, bus: jcan.Bus, logger: Logger, frame_id: hex, control: TimedOneAxisVelocityControl):
        super().__init__(bus=bus, logger=logger, frame_id=frame_id, control=control)
        self.start_time = 0.0

    def stop(self):
        """Stop the controller"""
        self.start_time = 0.0
        super().stop()

  
    def run_timed(self, run_time: float):
        control: TimedOneAxisVelocityControl = self.get_control()
        control.update_velocity(control.get_max_percent())
        control.set_time(run_time)
        self.start_time = time.time()

    def control_send_callback(self):
        """Get the frame to send over the CAN bus"""
        control: TimedOneAxisVelocityControl = self.get_control()

        curr_time = time.time()

        if curr_time - self.start_time >= control.get_time():
            self.get_logger().info('Pump Stopped: Timer Finished')
            self.stop()
            return
        
        super().control_send_callback()