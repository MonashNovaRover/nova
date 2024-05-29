#!/usr/bin/env python3

import jcan
from logging import Logger
from python_control.controls.OneAxisPositionControl import OneAxisPositionControl
from python_control.controllers.Controller import Controller
from python_control.controllers.Card import Card
from nova_interfaces.action import Stepper
from rclpy.action import CancelResponse


class StepperPositionController(Controller):
    TIMEOUT = 1200

    """Class to control the CMD card on the CAN bus"""
    def __init__(
            self, 
            card: Card, 
            max_value: int, 
            bus: jcan.Bus, 
            logger: Logger, 
            frame_id: hex, 
            control: OneAxisPositionControl,
    ):
        super().__init__(
            card=card, 
            max_value=max_value, 
            frame_id=frame_id, 
            control=control, bus=bus, 
            logger=logger)
        
        self.zeroing = False
        self.go_to = False
        self.setting = False

    def log_status(self):
        control = self.get_control()
        self.get_logger().info('Sensor Value: {0}, Goal Value: {1}, Goal Name: {2}'.format(control.get_current_position(), control.get_goal_position(), control.get_position_name()))

    def get_control(self) -> OneAxisPositionControl:
        return self.control

    def stop(self):
        """Stop the controller"""
        self.zeroing = False
        self.go_to = False
        self.setting = False
        self.get_control().stop()

    def zero(self):
        self.stop()
        self.zeroing = True
        self.get_control().zero()


    def go_to_position(self, name: str):
        self.stop()
        self.go_to = True
        self.get_control().update_position(name)

    def set_position(self, name: str):
        self.stop()
        self.setting = True
        self.get_control().update_position(name)


    def is_zeroing(self):
        return self.zeroing
    
    def is_going_to_position(self):
        return self.go_to
    
    def is_setting(self):
        return self.setting
    
    
    def feedback(self):
        feedback_msg = Stepper.Feedback()
        feedback_msg.current_position = self.get_control().get_current_position()
        feedback_msg.goal_position = self.get_control().get_goal_position()
        self.get_logger().debug('Feedback: {0}'.format(feedback_msg))
        return feedback_msg
    
    def feedback_loop(self, goal_handle):
        control = self.get_control()
        self.log_status()
        self.control_send_callback()

        try:
            frame = self.bus.receive_with_timeout(200)
            control.sensor_callbacks(frame)
        except OSError as _:
            pass
        feedback_msg = self.feedback()
        if not goal_handle.is_cancel_requested:
            goal_handle.publish_feedback(feedback_msg)

    def zeroing_action(self, goal_handle):
        self.get_logger().info('Zeroing Stepper')
        self.zero()

        i = 0
        while not self.get_control().is_zeroed() and i < self.TIMEOUT:
            if goal_handle.is_cancel_requested:
                self.get_logger().info('Canceling Stepper Goal')
                return False
            self.feedback_loop(goal_handle)
            i += 1

        self.log_status()
        
        success = self.get_control().is_zeroed()

        if success:
            self.get_logger().info('Zeroed Stepper')
            
        else:
            self.get_logger().error('Failed to zero Stepper')

        return success

    def go_to_position_action(self, goal_handle, goal_name):
        self.get_logger().info('Going to position: {0}'.format(goal_name))
        self.go_to_position(goal_name)
        
        i = 0
        while not self.get_control().is_at_position() and i < self.TIMEOUT:
            if goal_handle.is_cancel_requested:
                self.get_logger().info('Canceling Stepper Goal')
                return False
            self.feedback_loop(goal_handle)
            i += 1

        self.log_status()

        success = self.get_control().is_at_position()

        if success:
            self.get_logger().info('Successfully reached position: {0}'.format(goal_name))
            
        else:
            self.get_logger().error('Failed to reach position: {0}'.format(goal_name))

        return success
    
    def setting_position_action(self, goal_handle, goal_name):
        self.get_logger().info('Setting position: {0}'.format(goal_name))
        self.set_position(goal_name)
        
        i = 0
        while not self.get_control().is_at_position() and i < self.TIMEOUT:
            if goal_handle.is_cancel_requested:
                self.get_logger().info('Canceling Stepper Goal')
                return False
            self.feedback_loop(goal_handle)
            i += 1

        self.log_status()

        success = self.get_control().is_at_position()

        if success:
            self.get_logger().info('Successfully set position: {0}'.format(goal_name))
            
        else:
            self.get_logger().error('Failed to set position: {0}'.format(goal_name))

        return success

    def stepper_cancel_callback(self, goal_handle):
        self.get_logger().info('Canceling Stepper Goal')
        return CancelResponse.ACCEPT

    def stepper_action_callback(self, goal_handle):
        goal_name = goal_handle.request.goal
        action = goal_handle.request.action
        self.get_logger().info('Executing Stepper Goal: {0}'.format(goal_name))

        if not self.get_control().valid_position(goal_name):
            self.get_logger().error('Invalid command: {0}'.format(goal_name))
            goal_handle.abort()
            result = Stepper.Result()
            result.success = False
            return result

        success: bool
        if action == goal_handle.request.ZERO:
            success = self.zeroing_action(goal_handle)
        elif action == goal_handle.request.GO_TO:
            success = self.go_to_position_action(goal_handle, goal_name)
        elif action == goal_handle.request.SET:
            success = self.setting_position_action(goal_handle, goal_name)
        else:
            success = False
            self.get_logger().error('Invalid action: {0}'.format(action))

        self.stop()

        if not goal_handle.is_cancel_requested:
            if success:
                goal_handle.succeed()
            else:
                goal_handle.abort()
        else:
            goal_handle.canceled()

        result = Stepper.Result()
        result.success = success
        return result
    
    def control_send_callback(self):
        """Send the control frame over the CAN bus"""
        if self.is_zeroing() or self.is_going_to_position() or self.is_setting():
            super().control_send_callback()
        else:
            self.get_logger().debug("Controller is stopped, not sending frame")