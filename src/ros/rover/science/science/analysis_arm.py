#!/usr/bin/env python3

import rclpy, time
from python_control.ActivatedJoystickControllerNode import ActivatedJoystickControllerNode
from python_control.controllers.CMDVelocityController import CMDVelocityController
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl
from python_control.controls.Direction import Direction
from python_control.sensors.RangeSensor import RangeSensor
from python_control.limits.IntegerLimit import IntegerLimit
from input_interfaces.msg import InputJoystick
from sensor_msgs.msg import Range

"""
Analysis Arm will use stepper motor

Distance range of stepper = [0mm, 255mm]

Time of Flight Sensor (TOF)
Used as a limit
Publish its height to ROS topic => /science/analysis_arm

Controlled using joystick buttons
Left joystick x-axis => Move platform up/down (send max/min position) => heartbeat should handle stopping
When let go of joystick, send current position as goal position??

Left thumb up => Move platform up (small increment)
Right thumb button => Move platform down (small increment) + ignore TOF sensor
"""


class AnalysisArm(ActivatedJoystickControllerNode):

    # CAN BUS NAME
    # The name of the CAN bus to use
    CAN_BUS = "can1"

    # SENDING CARD IDS
    # Add any CONTROL FRAME / CARD IDS here
    CMD_ID_PARAM = "cmd_id"
    CMD_ID = 0x032

    # RECEIVING CARD IDS
    # Add any SENSOR FRAME / CARD IDS here
    TOF_FRAME_ID_PARAM = "tof_frame_id"
    TOF_FRAME_ID = 0x456 

    # CONTROL NAMES
    # Add any CONTROL names here
    PLATFORM_CONTROL_NAME = "platform"
    TOF_SENSOR_NAME = "tof"

    # CONTROL PARAMETERS
    # Max Speed as a Percentage (0.0 to 1.0)
    PLATFORM_MAX_PERCENT = 0.5
    MAX_PERCENT_PARAM = "max_percent"

    # CONTROL DIRECTIONS
    # Add any CONTROL DIRECTIONS here
    PLATFORM_UP = Direction.NEGATIVE
    PLATFORM_DOWN = Direction.POSITIVE

    # LIMIT PARAMETERS
    # Add any LIMIT parameters here
    TIME_OF_FLIGHT_OFFSET = 20
    TIME_OF_FLIGHT_MIN = 30
    TIME_OF_FLIGHT_MAX = 165

    TWITCH_SLEEP_TIME = 0.5


    def __init__(self):
        super(AnalysisArm, self).__init__(name="AnalysisArm", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        # Setting ROS parameters
        # This is done so that the parameters can be changed during runtime if desired
        self.declare_parameter(self.CMD_ID_PARAM, self.CMD_ID)
        self.declare_parameter(self.TOF_FRAME_ID_PARAM, self.TOF_FRAME_ID)
        self.declare_parameter(self.MAX_PERCENT_PARAM, self.PLATFORM_MAX_PERCENT)
        self.get_logger().info(f"CAN IDs: CMD = {self.get_parameter(self.CMD_ID_PARAM).value} TOF = {self.get_parameter(self.TOF_FRAME_ID_PARAM).value}")

        ## Add Flags as required
        self.twitch_enable = True
        self.twitch_button_released = True

        ## Add CAN ID Filters
        self.bus.set_id_filter([self.get_parameter(self.TOF_FRAME_ID_PARAM).value])

        ## Add Publishers
        self.tof_publisher = self.create_publisher(Range, "/science/analysis_arm", 10)

        ## Create sensors
        tof_sensor = RangeSensor(
            logger=logger,
            bus=self.bus,
            frame_id=self.get_parameter(self.TOF_FRAME_ID_PARAM).value,
            maximum=self.TIME_OF_FLIGHT_MAX,
            minimum=self.TIME_OF_FLIGHT_MIN,
            offset=self.TIME_OF_FLIGHT_OFFSET,
            publisher=self.tof_publisher,
            run_can=False
        )

        # Create limits
        platform_bottom_limit = IntegerLimit(
            logger=logger,
            bus=self.bus,
            is_maximum=False,
            limit_value=self.TIME_OF_FLIGHT_MIN,
            integer_sensor=tof_sensor
        )

        ## Create controls
        self.platform = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.get_parameter(self.MAX_PERCENT_PARAM).value,
            pos_limit=platform_bottom_limit,
        )

        ## Create controllers
        self.platform_controller = CMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.get_parameter(self.CMD_ID_PARAM).value,
            control=self.platform,
        )
        
        self.add_sensor(self.TOF_SENSOR_NAME, tof_sensor)

        ## Add the controllers to the node's of controllers
        self.add_controller(self.PLATFORM_CONTROL_NAME, self.platform_controller)

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
