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
    LIMIT_SWITCH_FRAME_ID = 0x4A2


    # CONTROL NAMES
    # Add any CONTROL names here
    SAMPLE_TRAY_NAME = "sample_tray"

    # CONTROL PARAMETERS
    # Positions
    SAMPLE_ONE_POS = 10
    SAMPLE_TWO_POS = 20
    CACHE_POS = 30
    CLEAN_POS = 40
    # Position Names
    ZERO = "zero"
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


    def __init__(self):
        super().__init__(name="URCSampleTray", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        ## Add Flags as required


        ## Add CAN ID Filters
        # self.bus.set_id_filter([])

        ## Add Publishers
        self.tof_publisher = self.create_publisher(Range, "/science/sample_tray", 10)

        # Add Actions
        self.go_to_action = ActionServer(self, Stepper, "/science/go_to_position", self.go_to_position_callback)

        ## Create sensors
        self.sample_tray_pos_sensor = IntegerSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.STEPPER_PCB_RECV,
            command_id=self.STEPPER_RECV_POS_COMMAND_ID,
            publisher=self.tof_publisher,
            initial_value=0,
        )

        self.sample_tray_zero_sensor = CommandSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.STEPPER_PCB_RECV,
            command_id=self.STEPPER_RECV_ZERO_COMMAND_ID,
        )    

        ## Create controls
        POSITIONS = {
            self.ZERO: 0,
            self.SAMPLE_ONE_NAME: self.SAMPLE_ONE_POS,
            self.SAMPLE_TWO_NAME: self.SAMPLE_TWO_POS,
            self.CACHE_NAME: self.CACHE_POS,
            self.CLEAN_NAME: self.CLEAN_POS,
        }

        self.sample_tray_control = OneAxisPositionControl(
            logger=logger,
            positions=POSITIONS,
            position_sensor=self.sample_tray_pos_sensor,
        )

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

        ## Add the controllers to the node's of controllers
        self.add_controller(self.SAMPLE_TRAY_NAME, self.sample_tray_controller)

        ## Start the CAN bus
        self.start_can()


    def go_to_position_callback(self, goal_handle):
        self.get_logger().info('Executing Stepper Go To: {0}'.format(goal_handle.request.command))
        command = goal_handle.request.command

        feedback_msg = Stepper.Feedback()
        feedback_msg.steps_left = self.sample_tray_control.distance_to_position()

        TIMEOUT = 60
        i = 0
        while abs(self.steps_left(command)) > 0 and not i < TIMEOUT:
            feedback_msg.steps_left = self.sample_tray_control.distance_to_position()
            self.get_logger().info('Feedback: steps_left {0}'.format(feedback_msg.steps_left))
            goal_handle.publish_feedback(feedback_msg)
            i += 1
            time.sleep(1)
    

        goal_handle.succeed()

        result = Stepper.Result()
        result.success = self.steps_left(command) > 0
        return result
    
    def zero_callback(self, goal_handle):
        # Implement pls
        pass
    

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