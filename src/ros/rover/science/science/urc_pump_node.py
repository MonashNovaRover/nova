#!/usr/bin/env python3


from python_control.controls.Direction import Direction
from python_control.controllers.TimedCMDVelocityController import TimedCMDVelocityController
from python_control.controls.TimedOneAxisVelocityControl import TimedOneAxisVelocityControl
import rclpy
from rclpy.action import ActionServer
from python_control.ControllerNode import ControllerNode
from input_interfaces.msg import InputJoystick


class URCPumps(ControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    PUMP_1_RUN_SEND_FRAME_ID = 0x022 # TODO: REPLACE value
    PUMP_2_RUN_SEND_FRAME_ID = 0x021 # TODO: REPLACE value

    # CONTROL NAMES
    # Add any CONTROL names here
    PUMP_1_NAME = "pump_1"
    PUMP_2_NAME = "pump_2"

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    PUMP_MAX_PERCENT = 0.75
    
    # CONTROL DIRECTIONS
    # Add any CONTROL DIRECTIONS here
    PUMP_FORWARDS = Direction.POSITIVE
    PUMP_BACKWARDS = Direction.NEGATIVE


    def __init__(self):
        super(URCPumps, self).__init__(name="URCAuger", can_bus=self.CAN_BUS)
        logger = self.get_logger()


        ## Create controls
        self.pump_1 = TimedOneAxisVelocityControl(
            logger=logger,
            max_percent=self.PUMP_MAX_PERCENT,
            direction=self.PUMP_FORWARDS,
        )

        self.pump_2 = TimedOneAxisVelocityControl(
            logger=logger,
            max_percent=self.PUMP_MAX_PERCENT,
            direction=self.PUMP_FORWARDS,
        )


        ## Create controllers
        self.pump_1_controller = TimedCMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.PUMP_1_RUN_SEND_FRAME_ID,
            control=self.pump_1,
        )

        self.pump_2_controller = TimedCMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.PUMP_2_RUN_SEND_FRAME_ID,
            control=self.pump_2,
        )


        ## Add the controllers to the node's of controllers
        self.add_controller(self.PUMP_1_NAME, self.pump_1_controller)
        self.add_controller(self.PUMP_2_NAME, self.pump_2_controller)

        # self.pump_action = ActionServer(self, Pump, self.SAMPLE_TRAY_ACTION, self.sample_tray_controller.stepper_action_callback)
  
        ## Start the CAN bus
        self.start_can()





    def joystick_l(self, joystick_l: InputJoystick):
        pass

    def joystick_r(self, joystick_r: InputJoystick):
        pass

def main():
    rclpy.init()
    node = URCPumps()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()