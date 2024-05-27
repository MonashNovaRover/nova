#!/usr/bin/env python3

from python_control.controls.OneAxisPositionControl import OneAxisPositionControl
from python_control.controllers.StepperPCBController import StepperPCBPositionController
from python_control.sensors.IntegerSensor import IntegerSensor
from python_control.ControllerNode import ControllerNode
import rclpy
from rclpy.action import ActionServer
from rclpy.executors import MultiThreadedExecutor
from nova_interfaces.action import Stepper

GAP_STEPS = -10

class URCCarousel(ControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # ROS ACTION
    # The name of the ROS action to use
    CAROUSEL_ACTION = "/science/carousel_action"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    STEPPER_PCB_SEND = 0x050

    # RECEIVING CARD IDS
    # Add any SENSOR FRAME / CARD IDS here
    STEPPER_PCB_RECV_POS = 0x455

    # NAMES
    # Add any CONTROL names here
    CAROUSEL_CONTROL = "carousel"
    # Add any SENSOR names here
    CAROUSEL_POS = "carousel_pos"

    # CONTROL PARAMETERS
    # Positions
    NUM_CUVETTES = 20

    # Position Names
    ZERO = OneAxisPositionControl.ZERO

    # SENDING COMMAND IDS
    # Add any CONTROL command ids here
    STEPPER_SEND_POS_COMMAND_ID = 0x01
    STEPPER_SEND_SET_COMMAND_ID = 0x02

    # RECEIVING COMMAND IDS
    # Add any SENSOR command ids here
    STEPPER_RECV_POS_COMMAND_ID = 0x01

    def __init__(self):
        super().__init__(name="URCCarousel", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        ## Add CAN ID Filters
        self.bus.set_id_filter([self.STEPPER_PCB_RECV_POS])

        ## Create sensors
        self.carousel_pos_sensor = IntegerSensor(
            bus=self.bus,
            logger=logger,
            frame_id=self.STEPPER_PCB_RECV_POS,
            command_id=self.STEPPER_RECV_POS_COMMAND_ID,
            initial_value=0,
        )

        ## Add sensors to the node
        self.add_sensor(self.CAROUSEL_POS, self.carousel_pos_sensor)


        ## Create controls
        POSITIONS = {
            self.ZERO: 0
        }

        for pos in range(1, self.NUM_CUVETTES + 1):
            POSITIONS[str(pos)] = (pos - 1) * GAP_STEPS


        self.carousel_control = OneAxisPositionControl(
            logger=logger,
            positions=POSITIONS,
            position_sensor=self.carousel_pos_sensor,
        )

        logger.info('Positions: {0}'.format(self.carousel_control.get_positions()))

        ## Create controllers
        self.carousel_controller = StepperPCBPositionController(
            logger=logger,
            bus=self.bus,
            frame_id=self.STEPPER_PCB_SEND,
            pos_command_id=self.STEPPER_SEND_POS_COMMAND_ID,
            set_command_id=self.STEPPER_SEND_SET_COMMAND_ID,
            control=self.carousel_control,
        )

        # Add Actions
        self.go_to_action = ActionServer(
            node=self, 
            action_type=Stepper,
            action_name=self.CAROUSEL_ACTION,
            execute_callback=self.carousel_controller.stepper_action_callback,
            cancel_callback=self.carousel_controller.stepper_cancel_callback,
        )

        ## Start the CAN bus
        self.start_can()


def main():
    rclpy.init()
    node = URCCarousel()
    rclpy.spin(node, executor=MultiThreadedExecutor())
    rclpy.shutdown()


if __name__ == "__main__":
    main()