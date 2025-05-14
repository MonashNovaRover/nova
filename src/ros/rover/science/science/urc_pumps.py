#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control for the URC Pumps
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: URCPumps
TOPICS: None
SERVICES: None
ACTIONS:
    - "/science/pumps_action"
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    science
AUTHOR(S):	???, Felicity Matthews
CREATION:	???
EDITED:		14/05/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import time
from python_control.controls.Direction import Direction
from python_control.controllers.TimedCMDVelocityController import TimedCMDVelocityController
from python_control.controls.TimedOneAxisVelocityControl import TimedOneAxisVelocityControl
import rclpy
from rclpy.action import ActionServer
from python_control.ControllerNode import ControllerNode
from nova_interfaces.action import Pumps


class URCPumps(ControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    CACHE_TO_SHOT_PUMP_RUN_SEND_FRAME_ID = 0x031
    SHOT_TO_CAROUSEL_PUMP_RUN_SEND_FRAME_ID = 0x032

    # CONTROL NAMES
    # Add any CONTROL names here
    CACHE_TO_SHOT_PUMP_NAME = "cache_to_shot_pump"
    SHOT_TO_CAROUSEL_PUMP_NAME = "shot_to_carousel_pump"

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    PUMP_MAX_PERCENT = 0.75
    
    # CONTROL DIRECTIONS
    # Add any CONTROL DIRECTIONS here
    PUMP_FORWARDS = Direction.POSITIVE
    PUMP_BACKWARDS = Direction.NEGATIVE

    # Pump ACTION
    PUMPS_ACTION = "/science/pumps_action"

    # Goals
    FILL_SHOTS = "fill_shots"
    FILL_CUVETTES_PRIME = "fill_cuvettes_prime"
    FILL_CUVETTES = "fill_cuvettes"

    TIMEOUT = 1200

    def __init__(self):
        super(URCPumps, self).__init__(name="URCPumps", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        self.ACTIONS = {
            self.FILL_SHOTS: self.fill_shots_action,
            self.FILL_CUVETTES_PRIME: self.fill_cuvettes_prime_action,
            self.FILL_CUVETTES: self.fill_cuvettes_action,
        }

        self.TIMED_PRESETS = {
            self.FILL_SHOTS: 10,
            self.FILL_CUVETTES_PRIME: 10,
            self.FILL_CUVETTES: 10,
        }

        ## Create controls
        self.cache_to_shot_pump = TimedOneAxisVelocityControl(
            logger=logger,
            max_percent=self.PUMP_MAX_PERCENT,
            direction=self.PUMP_FORWARDS,
        )

        self.shot_to_carousel_pump = TimedOneAxisVelocityControl(
            logger=logger,
            max_percent=self.PUMP_MAX_PERCENT,
            direction=self.PUMP_FORWARDS,
        )

        ## Create controllers
        self.cache_to_shot_pump_controller = TimedCMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.CACHE_TO_SHOT_PUMP_RUN_SEND_FRAME_ID,
            control=self.cache_to_shot_pump,
        )

        self.shot_to_carousel_pump_controller = TimedCMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.SHOT_TO_CAROUSEL_PUMP_RUN_SEND_FRAME_ID,
            control=self.cache_to_shot_pump,
        )

        self.pumps_action = ActionServer(self, Pumps, self.PUMPS_ACTION, self.pumps_goal_callback)

        ## Start the CAN bus
        self.start_can()


    def valid_action(self, action: str):
        return action in self.ACTIONS.keys()


    def log_status(self, pump, time, running_time):
        self.get_logger().info('Pump: {0}, Time: {1}, Running Time: {2}'.format(pump, time, running_time))
        
    def feedback(self, time_running, time_to_run):
        feedback_msg = Pumps.Feedback()
        feedback_msg.time_running = float(time_running)
        feedback_msg.time_to_run = float(time_to_run)
        return feedback_msg
    

    def elapsed_time(self, start_time):
        return time.time() - start_time
    

    def run_pump(self, goal_handle, controller):
        control = controller.get_control()        
        goal_name = goal_handle.request.pump

        if goal_handle.request.time_to_run:
            controller.run_timed(goal_handle.request.time_to_run)
        else:
            controller.run_timed(self.TIMED_PRESETS[goal_name])

        start_time = controller.get_start_time()

        i = 0
        while control.get_time() and i < self.TIMEOUT:
            running_time = self.elapsed_time(start_time)
            controller.control_send_callback()
            feedback_msg = self.feedback(running_time, control.get_time())
            goal_handle.publish_feedback(feedback_msg)
            self.log_status(goal_name, control.get_time(), running_time)
            time.sleep(0.1)
            i += 1

        success = running_time > control.get_time()

        if success:
            self.get_logger().info('Successfully ran pump: {0}'.format(goal_handle.request.pump))
            
        else:
            self.get_logger().error('Failed to run pump: {0}'.format(goal_handle.request.pump))

        return success


    def fill_shots_action(self, goal_handle):
        self.get_logger().info('Running Clean Fill Shots Pump')
        controller = self.cache_to_shot_pump_controller

        return self.run_pump(goal_handle, controller)
        

    def fill_cuvettes_prime_action(self, goal_handle):
        self.get_logger().info('Running Fill Cuvettes Prime Pump')
        controller = self.shot_to_carousel_pump_controller

        return self.run_pump(goal_handle, controller)

    
    def fill_cuvettes_action(self, goal_handle):
        self.get_logger().info('Running Fill Cuvettes Pump')
        controller = self.shot_to_carousel_pump_controller

        return self.run_pump(goal_handle, controller)
        
    
    def pumps_goal_callback(self, goal_handle):
        pump_action = goal_handle.request.pump
        self.get_logger().info('Executing Pump Action: {0}'.format(pump_action))

        if not self.valid_action(pump_action):
            self.get_logger().error('Invalid command: {0}'.format(pump_action))
            goal_handle.abort()
            result = Pumps.Result()
            result.success = False
            return result

        success: bool
        success = self.ACTIONS[pump_action](goal_handle)

        self.stop()

        if success:
            goal_handle.succeed()
        else:
            goal_handle.abort()

        result = Pumps.Result()
        result.success = success
        return result
    
    def stop(self):
        self.cache_to_shot_pump_controller.stop()
        self.shot_to_carousel_pump_controller.stop()


def main():
    rclpy.init()
    node = URCPumps()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()