#!/usr/bin/env python3

from python_control.controls.OneAxisPositionControl import OneAxisPositionControl
from python_control.controllers.StepperPCBController import StepperPCBPositionController
from python_control.sensors.IntegerSensor import IntegerSensor
from python_control.sensors.CommandSensor import CommandSensor
from python_control.ControllerNode import ControllerNode
import rclpy
from input_interfaces.msg import InputJoystick
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

    def __init__(self):
        super().__init__(name="URCSampleTray", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        ## Add CAN ID Filters
        self.bus.set_id_filter([self.STEPPER_PCB_RECV])

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

        self.sample_tray_control = OneAxisPositionControl(
            logger=logger,
            positions=POSITIONS,
            position_sensor=self.sample_tray_pos_sensor,
            zero_sensor=self.sample_tray_zero_sensor,
        )

        logger.info('Positions: {0}'.format(self.sample_tray_control.get_positions()))

        ## Create controllers
        self.sample_tray_controller = StepperPCBPositionController(
            logger=logger,
            bus=self.bus,
            frame_id=self.STEPPER_PCB_SEND,
            pos_command_id=self.STEPPER_SEND_POS_COMMAND_ID,
            zero_command_id=self.STEPPER_SEND_ZERO_COMMAND_ID,
            control=self.sample_tray_control,
        )

        # Add Actions
        self.go_to_action = ActionServer(self, Stepper, "/science/sample_tray_action", self.sample_tray_controller.stepper_action_callback)

        ## Start the CAN bus
        self.start_can()


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