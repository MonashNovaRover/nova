#!/usr/bin/env python3

from python_control.limits.LimitSwitchLimit import LimitSwitchLimit
from python_control.controls.Direction import Direction
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl
from python_control.controllers.CMDVelocityController import CMDVelocityController
from python_control.sensors.CommandSensor import CommandSensor
from python_control.JoystickControllerNode import JoystickControllerNode
import rclpy
from input_interfaces.msg import InputJoystick


class URCAuger(JoystickControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    AUGER_ACTUATION_CANID_PARAM = "auger_actuation_canid"
    AUGER_ACTUATION_SEND_FRAME_ID = 0x0C2
    AUGER_DRILL_CANID_PARAM = "auger_drill_canid"
    AUGER_DRILL_SEND_FRAME_ID = 0x0C1
    
    # RECEIVING CARD IDS
    # Add any SENSOR FRAME / CARD IDS here
    AUGER_LIMIT_RECV_ID = 0x452

    # CONTROL NAMES
    # Add any CONTROL names here
    AUGER_ACTUATION_NAME = "auger_actuation"
    AUGER_DRILL_NAME = "auger_drill"

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    AUGER_ACTUATION_MAX_PERCENT = 0.75
    AUGER_DRILL_MAX_PERCENT = 0.75

    # SENDING COMMAND IDS
    # Add any CONTROL command ids here
    STEPPER_SEND_COMMAND_ID = 0x01

    # RECEIVING COMMAND IDS
    # Add any SENSOR command ids here
    AUGER_RECV_LIMIT_BOTTOM_COMMAND_ID = 0x01
    
    # CONTROL DIRECTIONS
    # Add any CONTROL DIRECTIONS here
    AUGER_ACTUATION_UP = Direction.NEGATIVE
    AUGER_ACTUATION_DOWN = Direction.POSITIVE
    AUGER_DRILL_CLOCKWISE = Direction.POSITIVE
    AUGER_DRILL_COUNTERCLOCKWISE = Direction.NEGATIVE

    def __init__(self):
        super(URCAuger, self).__init__(name="URCAuger", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        # Setting ROS parameters
        self.declare_parameter(self.AUGER_ACTUATION_CANID_PARAM, self.AUGER_ACTUATION_SEND_FRAME_ID)
        self.declare_parameter(self.AUGER_DRILL_CANID_PARAM, self.AUGER_DRILL_SEND_FRAME_ID)
        self.get_logger().info(f"CAN IDs: Actuation = {self.get_parameter(self.AUGER_ACTUATION_CANID_PARAM).value} Drill = {self.get_parameter(self.AUGER_DRILL_CANID_PARAM).value}")

        ## Add CAN ID Filters
        self.bus.set_id_filter([self.AUGER_LIMIT_RECV_ID])

        ## Create sensors
        self.bottom_limit_hall_effect = CommandSensor(
            logger=logger,
            bus=self.bus,
            frame_id=self.AUGER_LIMIT_RECV_ID,
            command_id=self.AUGER_RECV_LIMIT_BOTTOM_COMMAND_ID,
            run_can=False
        )

        # Create limits
        self.auger_bottom_limit = LimitSwitchLimit(
            logger=logger,
            bus=self.bus,
            limit_switch=self.bottom_limit_hall_effect,
        )

        ## Create controls
        self.auger_actuation = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.AUGER_ACTUATION_MAX_PERCENT,
            direction=self.AUGER_ACTUATION_UP,
            neg_limit=self.auger_bottom_limit,
        )
        self.auger_drill = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.AUGER_DRILL_MAX_PERCENT,
            direction=self.AUGER_DRILL_CLOCKWISE,
        )


        ## Create controllers
        self.auger_actuation_controller = CMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.get_parameter(self.AUGER_ACTUATION_CANID_PARAM).value,
            control=self.auger_actuation
        )
        self.auger_drill_controller = CMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.get_parameter(self.AUGER_DRILL_CANID_PARAM).value,
            control=self.auger_drill
        )

        ## Add the controllers to the node's of controllers
        self.add_controller(self.AUGER_ACTUATION_NAME, self.auger_actuation_controller)
        self.add_controller(self.AUGER_DRILL_NAME, self.auger_drill_controller)

        ## Start the CAN bus
        self.start_can()

    def update_auger_actuation(self, joystick_r: InputJoystick):
        # Auger height direction is determined by the right joystick's x-axis direction
        self.auger_actuation.update_direction(self.AUGER_ACTUATION_UP if joystick_r.ax_stick_x <= 0 else self.AUGER_ACTUATION_DOWN)

        # Auger velocity is determined by the right joystick's x-axis magnitude
        if joystick_r.btn_thumb_d_state >= 1 or joystick_r.ax_stick_x < 0:
            self.bottom_limit_hall_effect.set_sensor_value(False)
            self.auger_bottom_limit.update_limit_hit(False)
        self.auger_actuation.update_velocity(velocity=abs(joystick_r.ax_stick_x))
    

    def update_auger_drill(self, joystick_r: InputJoystick):
        # Drill spin direction is determined by the right joystick thumb buttons
        # Thumb right = clockwise, Thumb left = counterclockwise
        if joystick_r.btn_thumb_r_state >= 1:
            self.auger_drill.update_direction(self.AUGER_DRILL_CLOCKWISE)
        elif joystick_r.btn_thumb_l_state >= 1:
            self.auger_drill.update_direction(self.AUGER_DRILL_COUNTERCLOCKWISE)
        
        # Drill spin velocity is determined by the right joystick trigger
        if joystick_r.btn_thumb_u_state >= 1:
            self.auger_drill.update_velocity(1.0)
        else:
            self.auger_drill.update_velocity(0)

    def joystick_l(self, joystick_l: InputJoystick):
        pass

    def joystick_r(self, joystick_r: InputJoystick):
        self.update_auger_actuation(joystick_r)
        self.update_auger_drill(joystick_r)

def main():
    rclpy.init()
    node = URCAuger()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
