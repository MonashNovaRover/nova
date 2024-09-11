#!/usr/bin/env python3

from python_control.controls.OneAxisPositionControl import OneAxisPositionControl
from python_control.controllers.StepperPCBController import StepperPCBPositionController
from python_control.sensors.IntegerSensor import IntegerSensor
from python_control.sensors.CommandSensor import CommandSensor
from python_control.ControllerNode import ControllerNode
import rclpy
from rclpy.action import ActionServer
from rclpy.executors import MultiThreadedExecutor
from nova_interfaces.action import Stepper


def meters_to_steps(distance_m, ticks_per_rev, lead_screw_thread_pitch_m):
    """
    Convert a distance to steps
    :param distance: The distance to convert in meters
    :param ticks_per_rev: The number of ticks per revolution of the stepper motor
    :param lead_screw_thread_pitch: The pitch of the lead screw in meters
    :return: The number of steps required to move the distance
    """
    return int(distance_m * ticks_per_rev / lead_screw_thread_pitch_m)

THREAD_PITCH_M = 0.008
TICKS_PER_REV = 200
GAP = 0.084

class URCSampleTray(ControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # ROS ACTION
    # The name of the ROS action to use
    SAMPLE_TRAY_ACTION = "/science/sample_tray_action"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    STEPPER_PCB_SEND = 0x060

    # RECEIVING CARD IDS
    # Add any SENSOR FRAME / CARD IDS here
    STEPPER_PCB_RECV_POS = 0x465
    STEPPER_PCB_RECV_ZERO = 0x460


    # NAMES
    # Add any CONTROL names here
    SAMPLE_TRAY_CONTROL = "sample_tray"
    SAMPLE_TRAY_POS = "sample_tray_pos"
    SAMPLE_TRAY_ZERO = "sample_tray_zero"

    # 84mm Apart
    # Distance (meters) to Steps (ticks) Conversion
    # n_ticks = desired_d * ticks_per_rev / lead_screw_thread_pitch (keep all in SI units)
    # ticks_per_rev = 200 For out lead screws, the thread pitch is 8mm. (0.008 meters)

    # CONTROL PARAMETERS
    # Positions
    SAMPLE_TWO_POS = meters_to_steps(0 * GAP, TICKS_PER_REV, THREAD_PITCH_M)
    CLEAN_POS = meters_to_steps(1 * GAP, TICKS_PER_REV, THREAD_PITCH_M)
    AUGER_POS = meters_to_steps(2 * GAP, TICKS_PER_REV, THREAD_PITCH_M)
    SAMPLE_ONE_POS =  meters_to_steps(3 * GAP, TICKS_PER_REV, THREAD_PITCH_M)
    CACHE_POS = meters_to_steps(4 * GAP, TICKS_PER_REV, THREAD_PITCH_M)

    # Position Names
    ZERO = OneAxisPositionControl.ZERO
    SAMPLE_ONE_NAME = "sample_one"
    SAMPLE_TWO_NAME = "sample_two"
    AUGER_NAME = "auger"
    CACHE_NAME = "cache"
    CLEAN_NAME = "clean"

    # SENDING COMMAND IDS
    # Add any CONTROL command ids here
    STEPPER_SEND_POS_COMMAND_ID = 0x01
    STEPPER_SEND_ZERO_COMMAND_ID = 0x03

    # RECEIVING COMMAND IDS
    # Add any SENSOR command ids here
    STEPPER_RECV_POS_COMMAND_ID = 0x01
    STEPPER_RECV_ZERO_COMMAND_ID = 0x01

    def __init__(self):
        super().__init__(name="URCSampleTray", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        ## Add CAN ID Filters
        self.bus.set_id_filter([self.STEPPER_PCB_RECV_POS, self.STEPPER_PCB_RECV_ZERO])

        ## Create sensors
        self.sample_tray_pos_sensor = IntegerSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.STEPPER_PCB_RECV_POS,
            command_id=self.STEPPER_RECV_POS_COMMAND_ID,
            initial_value=0,
        )

        self.sample_tray_zero_sensor = CommandSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.STEPPER_PCB_RECV_ZERO,
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
            self.AUGER_NAME: self.AUGER_POS,
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
        self.go_to_action = ActionServer(
            node=self, 
            action_type=Stepper, 
            action_name=self.SAMPLE_TRAY_ACTION, 
            execute_callback=self.sample_tray_controller.stepper_action_callback, 
            cancel_callback=self.sample_tray_controller.stepper_cancel_callback,
        )

        ## Start the CAN bus
        self.start_can()

def main():
    rclpy.init()
    node = URCSampleTray()
    rclpy.spin(node, executor=MultiThreadedExecutor())
    rclpy.shutdown()


if __name__ == "__main__":
    main()