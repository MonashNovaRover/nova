#!/usr/bin/env python3

from python_control.sensors.LimitSwitchSensor import LimitSwitchSensor
from python_control.limits.LimitSwitchLimit import LimitSwitchLimit
from python_control.controls.Direction import Direction
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl
from python_control.controllers.CMDVelocityController import CMDVelocityController
from python_control.sensors.CommandSensor import CommandSensor
import rclpy
from python_control.ControllerNode import ControllerNode
from input_interfaces.msg import InputJoystick


class URCAuger(ControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    MIXER_1_SEND_FRAME_ID = 0x041
    MIXER_2_SEND_FRAME_ID = 0x042

    # CONTROL NAMES
    # Add any CONTROL names here
    MIXER_1_NAME = "mixer_1"
    MIXER_2_NAME = "mixer_2"

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    MIXER_1_MAX_PERCENT = 0.75
    MIXER_2_MAX_PERCENT = 0.75
    
    # CONTROL DIRECTIONS
    # Add any CONTROL DIRECTIONS here
    MIXER_1_CLOCKWISE = Direction.POSITIVE
    MIXER_1_DOWN = Direction.NEGATIVE
    MIXER_2_CLOCKWISE = Direction.POSITIVE
    MIXER_2_COUNTERCLOCKWISE = Direction.NEGATIVE    

    def __init__(self):
        super(URCAuger, self).__init__(name="URCMixers", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        # ## Add Publishers
        # self.bme_publisher = self.create_publisher(, "/science/", 10)

        # ## Create Sensors
        # self.temperature = IntegerSensor()


        ## Create controllers
        self.mixer_1_controller = CMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.MIXER_1_SEND_FRAME_ID,
            control=self.mixer_1
        )
        self.mixer_2_controller = CMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.MIXER_2_SEND_FRAME_ID,
            control=self.mixer_2
        )

        ## Add the controllers to the node's of controllers
        self.add_controller(self.MIXER_1_NAME, self.mixer_1_controller)
        self.add_controller(self.MIXER_2_NAME, self.mixer_2_controller)

        ## Start the CAN bus
        self.start_can()



    def joystick_l(self, joystick_l: InputJoystick):
        pass

    def joystick_r(self, joystick_r: InputJoystick):
        pass

def main():
    rclpy.init()
    node = URCAuger()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()