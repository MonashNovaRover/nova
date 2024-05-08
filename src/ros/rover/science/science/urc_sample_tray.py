#!/usr/bin/env python3

import time
from python_control.controls.OneAxisPositionControl import OneAxisPositionControl
from python_control.controllers.StepperPCBController import StepperPCBPositionController
from python_control.sensors.IntegerSensor import IntegerSensor
from python_control.sensors.CommandSensor import CommandSensor
import rclpy
from python_control.ControllerNode import ControllerNode
from input_interfaces.msg import InputJoystick
from sensor_msgs.msg import Range
from rclpy.action import ActionServer
from nova_interfaces.action import Stepper


class URCSampleTray(ControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    STEPPER_PCB_SEND = 0x006

    # RECEIVING CARD IDS
    # Add any SENSOR FRAME / CARD IDS here
    STEPPER_PCB_RECV = 0x456


    # NAMES
    # Add any CONTROL names here
    SAMPLE_TRAY_CONTROL = "sample_tray"
    SAMPLE_TRAY_POS = "sample_tray_pos"
    SAMPLE_TRAY_ZERO = "sample_tray_zero"


    # CONTROL PARAMETERS
    # Positions
    SAMPLE_ONE_POS = 0
    SAMPLE_TWO_POS = 20
    CACHE_POS = 30
    CLEAN_POS = 40
    # Position Names
    ZERO = OneAxisPositionControl.ZERO
    SAMPLE_ONE_NAME = "sample_one"
    SAMPLE_TWO_NAME = "sample_two"
    CACHE_NAME = "cache"
    CLEAN_NAME = "clean"

    # SENDING COMMAND IDS
    # Add any CONTROL command ids here
    STEPPER_SEND_POS_COMMAND_ID = 0x01
    STEPPER_SEND_ZERO_COMMAND_ID = 0x03

    # RECEIVING COMMAND IDS
    # Add any SENSOR command ids here
    STEPPER_RECV_POS_COMMAND_ID = 0x01
    STEPPER_RECV_ZERO_COMMAND_ID = 0x03

    # TIMEOUT
    TIMEOUT = 1000

    def __init__(self):
        super().__init__(name="URCSampleTray", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        ## Add CAN ID Filters
        self.bus.set_id_filter([self.STEPPER_PCB_RECV])

        # Add Actions
        self.go_to_action = ActionServer(self, Stepper, "/science/sample_tray_action", self.stepper_action_callback)

        ## Create sensors
        self.sample_tray_pos_sensor = IntegerSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.STEPPER_PCB_RECV,
            command_id=self.STEPPER_RECV_POS_COMMAND_ID,
            initial_value=0,
        )

        self.sample_tray_zero_sensor = CommandSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.STEPPER_PCB_RECV,
            command_id=self.STEPPER_RECV_ZERO_COMMAND_ID,
            run_can=False,
        )    

        ## Add sensors to the node
        self.add_sensor(self.SAMPLE_TRAY_POS, self.sample_tray_pos_sensor)
        self.add_sensor(self.SAMPLE_TRAY_ZERO, self.sample_tray_zero_sensor)

        ## Create controls
        POSITIONS = {
            self.ZERO: 0,
            self.SAMPLE_ONE_NAME: self.SAMPLE_ONE_POS,
            self.SAMPLE_TWO_NAME: self.SAMPLE_TWO_POS,
            self.CACHE_NAME: self.CACHE_POS,
            self.CLEAN_NAME: self.CLEAN_POS,
        }

        logger.info('Positions: {0}'.format(POSITIONS))

        self.sample_tray_control = OneAxisPositionControl(
            logger=logger,
            positions=POSITIONS,
            position_sensor=self.sample_tray_pos_sensor,
        )

        logger.info('Positions Retrieved: {0}'.format(self.sample_tray_control.get_positions()))

        ## Create controllers
        self.sample_tray_controller = StepperPCBPositionController(
            logger=logger,
            bus=self.bus,
            frame_id=self.STEPPER_PCB_SEND,
            pos_command_id=self.STEPPER_SEND_POS_COMMAND_ID,
            zero_command_id=self.STEPPER_SEND_ZERO_COMMAND_ID,
            control=self.sample_tray_control,
            zero_sensor=self.sample_tray_zero_sensor,
        )

        ## Start the CAN bus
        self.start_can()

    def log_status(self):
        control = self.sample_tray_control
        self.get_logger().info('Sensor Value: {0}, Goal Value: {1}, Goal Name: {2}'.format(control.get_current_position(), control.get_goal_position(), control.get_position_name()))


    def feedback(self):
        feedback_msg = Stepper.Feedback()
        feedback_msg.current_position = self.sample_tray_control.get_current_position()
        feedback_msg.goal_position = self.sample_tray_control.get_goal_position()
        self.get_logger().debug('Feedback: {0}'.format(feedback_msg))
        return feedback_msg
    
    def feedback_loop(self, goal_handle):
        self.log_status()
        self.sample_tray_controller.control_send_callback()

        try:
            frame = self.bus.receive_with_timeout(200)
            self.sample_tray_pos_sensor.frame_callback(frame)
            self.sample_tray_zero_sensor.frame_callback(frame)
        except OSError as _:
            pass
        feedback_msg = self.feedback()
        goal_handle.publish_feedback(feedback_msg)

    def zeroing(self, goal_handle):
        self.get_logger().info('Zeroing Stepper')
        self.sample_tray_controller.zero()

        i = 0
        while self.sample_tray_controller.is_zeroing() and i < self.TIMEOUT:
            self.feedback_loop(goal_handle)
            i += 1

        self.log_status()
        if self.sample_tray_controller.is_zeroing():
            self.get_logger().error('Failed to zero Stepper')
            self.sample_tray_controller.stop()
            return False
        
        self.get_logger().info('Zeroed Stepper')
        return True

    def go_to_position(self, goal_handle, goal_name):
        self.get_logger().info('Going to position: {0}'.format(goal_name))
        self.sample_tray_controller.go_to_position(goal_name)
        
        i = 0
        while not self.sample_tray_control.is_at_position() and i < self.TIMEOUT:
            self.feedback_loop(goal_handle)
            i += 1

        self.log_status()

        if not self.sample_tray_control.is_at_position():
            self.get_logger().error('Failed to reach position: {0}'.format(goal_name))
            self.sample_tray_controller.stop()
            return False
        
        self.get_logger().info('Reached position: {0}'.format(goal_name))
        return True


    def stepper_action_callback(self, goal_handle):
        goal_name = goal_handle.request.goal
        self.get_logger().info('Executing Stepper Goal: {0}'.format(goal_name))

        if not self.sample_tray_control.valid_goal(goal_name):
            self.get_logger().error('Invalid command: {0}'.format(goal_name))
            goal_handle.abort()
            result = Stepper.Result()
            result.success = False
            return result

        success: bool
        if goal_name == self.ZERO:
            success = self.zeroing(goal_handle)
        else:
            success = self.go_to_position(goal_handle, goal_name)

        if success:
            goal_handle.succeed()
        else:
            goal_handle.abort()

        result = Stepper.Result()
        result.success = success
        return result
    

    def joystick_l(self, joystick_l: InputJoystick):
        pass

    def joystick_r(self, joystick_r: InputJoystick):
        pass

def main():
    rclpy.init()
    node = URCSampleTray()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()