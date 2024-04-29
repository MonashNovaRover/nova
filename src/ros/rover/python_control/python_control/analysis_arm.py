#!/usr/bin/env python3

import rclpy, time
from control.ControllerNode import ControllerNode
from control.classes.cards.CMDCardController import CMDCardController
from control.classes.controls.OneAxisControl import Direction, OneAxisControl
from control.classes.sensors.IntegerSensor import IntegerSensor
from control.classes.limits.IntegerLimit import IntegerLimit
from control.classes.limits.LimitSwitchLimit import LimitSwitchLimit
from control.classes.sensors.LimitSwitchSensor import LimitSwitchSensor
from core.msg import InputJoystick


class AnalysisArm(ControllerNode):

    CAN_BUS = "can1"
    CMD_ID = 0x10
    PLATFORM_MAX_PERCENT = 1.0

    TOF_FRAME_ID = 0x4A1
    LIMIT_SWITCH_FRAME_ID = 0x4A2
    LIMIT_SWITCH_COMMAND_ID = 0x01

    PLATFORM_DOWN = Direction.POSITIVE
    PLATFORM_UP = Direction.NEGATIVE

    TWITCH_SLEEP_TIME = 0.2


    def __init__(self):
        super(AnalysisArm, self).__init__(name="AnalysisArm", can_bus=self.CAN_BUS)
        self.twitch_enable = True
        self.twitch_button_released = True

        ## Add CAN ID Filters
        self.bus.set_id_filter([self.TOF_FRAME_ID, self.LIMIT_SWITCH_FRAME_ID])

        ## Create sensors
        tof_sensor = IntegerSensor(
            logger=self.get_logger(),
            bus=self.bus,
            frame_id=self.TOF_FRAME_ID,
        )

        limit_switch_top = LimitSwitchSensor(
            logger=self.get_logger(),
            bus=self.bus,
            frame_id=self.LIMIT_SWITCH_FRAME_ID,
            command_id=self.LIMIT_SWITCH_COMMAND_ID,
        )

        # Create limits
        platform_bottom_limit = IntegerLimit(
            logger=self.get_logger(),
            bus=self.bus,
            maximum=False,
            limit_value=10,
            integer_sensor=tof_sensor
        )

        platform_top_limit = LimitSwitchLimit(
            logger=self.get_logger(),
            bus=self.bus,
            limit_switch=limit_switch_top
        )

        ## Create controls
        self.platform = OneAxisControl(
            max_percent=self.PLATFORM_MAX_PERCENT,
            pos_limit=platform_bottom_limit,
            neg_limit=platform_top_limit,
        )

        ## Create controllers
        self.platform_controller = CMDCardController(
            logger=self.get_logger(),
            bus=self.bus,
            card_id=self.CMD_ID,
            control=self.platform,
        )

        ## Add the controllers to the node's of controllers
        self.add_controller("platform", self.platform_controller)

        ## Start the CAN bus
        self.start_can()

    def update_platform_height(self, joystick_l: InputJoystick):
        # analysis platform height direction is determined by the right joystick's x-axis direction
        self.platform.update_direction(self.PLATFORM_DOWN if joystick_l.ax_stick_x >= 0 else self.PLATFORM_UP)
        self.platform.update_velocity(velocity=abs(joystick_l.ax_stick_x))

        # guard case if twitch is already being performed
        if not self.twitch_enable:
            return
        elif not self.twitch_button_released:
            # check if the joystick has been released
            if joystick_l.btn_thumb_l_state < 1 and joystick_l.btn_thumb_r_state < 1:
                self.get_logger().info("Released twitch button")
                self.twitch_button_released = True
            return

        # button override time of flight
        # allows operators to lower the platform even 
        # if the time of flight sensor is reading the 0 / reached bottom
        if joystick_l.btn_thumb_l_state >= 1:
            self.get_logger().info("Twitch down begin")
            self.twitch(self.PLATFORM_DOWN)
            self.get_logger().info("Twitch down end")
        elif joystick_l.btn_thumb_r_state >= 1:
            self.get_logger().info("Twitch up begin")
            self.twitch(self.PLATFORM_UP)
            self.get_logger().info("Twitch up end")
    
    def twitch(self, direction: Direction):
        self.platform.update_direction(direction)
        self.platform.update_velocity(velocity=0.8, ignore_limits=True)
        self.twitch_enable = False
        self.twitch_button_released = False
        time.sleep(self.TWITCH_SLEEP_TIME)
        self.twitch_enable = True

    def joystick_l(self, joystick_l: InputJoystick):
        self.update_platform_height(joystick_l)

    def joystick_r(self, joystick_r: InputJoystick):
        pass

def main():
    rclpy.init()
    node = AnalysisArm()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()