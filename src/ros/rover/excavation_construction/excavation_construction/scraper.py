#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Scraper using Joysticks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: scraper
TOPICS:
  - subscriber: /inputs/input_joystick_l [InputJoystick]
  - subscriber: /inputs/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: control
COMMAND: ros2 run control scraper.py
RUN ON: Rover
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUTHOR(S):	Tristan Clark, Felicity Matthews
CREATION:	02/02/2024
EDITED:		20/02/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""
from python_control.controls.Direction import Direction
from python_control.controllers.Card import Card
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl
from python_control.controllers.CMDVelocityController import CMDVelocityController
from python_control.controllers.JonoVelocityController import JonoVelocityController
from python_control.JoystickControllerNode import JoystickControllerNode
import rclpy, jcan

# import the joystick ROS message we are listening to
from input_interfaces.msg import InputJoystick

class ScraperNode(JoystickControllerNode):
    # can bus
    CAN_BUS = "can0"
    # card IDs
    CARD = Card.CMD
    # only used when card = CMD
    CMD_ID_ARM = 0x0C1
    CMD_ID_SCOOP = 0x0D1
    CMD_ID_BUCKET = 0x0C2
    # only used when card = JONO
    JONO_ID_ARM = 0x0A0
    JONO_ID_SCOOP = 0x0A0
    JONO_ID_BUCKET = 0x0A0
    JONO_ID_LIMIT_SWITCH = 0x4A2
    # jono commands
    JONO_ARM_FORWARDS = 0x04
    JONO_ARM_BACKWARDS = 0x03
    JONO_SCOOP_FORWARDS = 0x06
    JONO_SCOOP_BACKWARDS = 0x05
    JONO_BUCKET_OPEN = 0x01
    JONO_BUCKET_CLOSE = 0x02

    # directions
    ARM_FORWARDS = Direction.POSITIVE
    ARM_BACKWARDS = Direction.NEGATIVE
    SCOOP_FORWARDS = Direction.POSITIVE
    SCOOP_BACKWARDS = Direction.NEGATIVE
    BUCKET_CLOSE = Direction.POSITIVE
    BUCKET_OPEN = Direction.NEGATIVE

    # max_velocity percent
    ARM_MAX_VELOCITY_PERCENT = 1.0
    SCOOP_MAX_VELOCITY_PERCENT = 1.0
    BUCKET_MAX_VELOCITY_PERCENT = 0.6

    # limit switch
    BUCKET_LIMIT_SWITCH_CLOSED = 0x01
    LIMIT_SWITCH_CLEAR = 0x00
    LIMIT_SWITCH_HIT = 0xFF # used to be 0x01

    # ROS param names
    CAN_BUS_PARAM = "can_bus"
    CARD_TYPE_PARAM = "card_type"
    ARM_MAX_VEL_PERCENT_PARAM = "arm_max_vel_percent"
    SCOOP_MAX_VEL_PERCENT_PARAM = "scoop_max_vel_percent"
    BUCKET_MAX_VEL_PERCENT_PARAM = "bucket_max_vel_percent"

    def __init__(self):
        super().__init__("scraper", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        # Setting ROS parameters
        # This is done so that the parameters can be changed during runtime if desired
        self.declare_parameter(self.CARD_TYPE_PARAM, self.CARD.value)

        self.declare_parameter(self.ARM_MAX_VEL_PERCENT_PARAM, self.ARM_MAX_VELOCITY_PERCENT)
        self.declare_parameter(self.SCOOP_MAX_VEL_PERCENT_PARAM, self.SCOOP_MAX_VELOCITY_PERCENT)
        self.declare_parameter(self.BUCKET_MAX_VEL_PERCENT_PARAM, self.BUCKET_MAX_VELOCITY_PERCENT)

        # Create the custom control classes
        # Arm control
        self.arm = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.get_parameter(self.ARM_MAX_VEL_PERCENT_PARAM).value,
            direction=self.ARM_FORWARDS,
        )
        # Scoop control
        self.scoop = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.get_parameter(self.SCOOP_MAX_VEL_PERCENT_PARAM).value,
            direction=self.SCOOP_FORWARDS,
        )
        # Bucket control
        self.bucket = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.get_parameter(self.BUCKET_MAX_VEL_PERCENT_PARAM).value,
            direction=self.BUCKET_OPEN,
        )

        self.velocity_multiplier = 0.0

        if self.get_parameter(self.CARD_TYPE_PARAM).value == Card.JONO.value:
            self.setup_jono_controllers()
        elif self.get_parameter(self.CARD_TYPE_PARAM).value == Card.CMD.value:
            self.setup_cmd_controllers()
        else:
            raise ValueError(f"Unknown card type: {self.get_parameter(self.CARD_TYPE_PARAM).value}")

        self.bus.set_id_filter([self.JONO_ID_LIMIT_SWITCH])
        self.bus.add_callback(self.JONO_ID_LIMIT_SWITCH, self.callback_receive_can_limit_switch)

        self.start_can()
        self.timer_jcan = self.create_timer(0.05, self.callback_send_can_commands)

        self.get_logger().info(f"Scraper started on {self.get_parameter(self.CAN_BUS_PARAM).value} using {self.get_parameter(self.CARD_TYPE_PARAM).value} card type")

    def setup_jono_controllers(self):
        logger = self.get_logger()
        # Scraper card controllers for Jono Card
        # Arm controller
        self.arm_controller = JonoVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.JONO_ID_ARM,
            pos_command=self.JONO_ARM_FORWARDS, 
            neg_command=self.JONO_ARM_BACKWARDS, 
            control=self.arm
        )
        # Scoop controller
        self.scoop_controller = JonoVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.JONO_ID_SCOOP,
            pos_command=self.JONO_SCOOP_FORWARDS, 
            neg_command=self.JONO_SCOOP_BACKWARDS, 
            control=self.scoop
        )
        # Bucket controller
        self.bucket_controller = JonoVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.JONO_ID_BUCKET,
            pos_command=self.JONO_BUCKET_OPEN, 
            neg_command=self.JONO_BUCKET_CLOSE, 
            control=self.bucket
        )
    
    def setup_cmd_controllers(self):
        logger=self.get_logger()
        # Scraper card controllers for CMD Card
        # Arm controller
        self.arm_controller = CMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.CMD_ID_ARM,
            control=self.arm,
        )
        # Scoop controller
        self.scoop_controller = CMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.CMD_ID_SCOOP,
            control=self.scoop,
        )
        # Bucket controller
        self.bucket_controller = CMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.CMD_ID_BUCKET,
            control=self.bucket,
        )

    def callback_receive_can_limit_switch(self, frame: jcan.Frame):
        """
        Callback for when the limit switch is hit
        """
        try:
            self.get_logger().debug(f"Received {frame.id} {frame.data}")

            if frame.id == self.JONO_ID_LIMIT_SWITCH and frame.data[0] == self.BUCKET_LIMIT_SWITCH_CLOSED:
                if frame.data[1] == self.LIMIT_SWITCH_CLEAR:
                    self.bucket.update_limit_pos(False)
                else:
                    self.get_logger().debug("Limit switch hit")
                    self.bucket.update_limit_pos(True)
            else:
                self.get_logger().warn("Received unknown frame")

        except Exception as e:
            self.get_logger().error(f"Error receiving CAN message: {e}")

    def callback_send_can_commands(self):
        """
        Take current internal state and publish over CAN
        Sends can commands for scraper scoop, scraper arm together
        """
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        armFrame = self.arm_controller.get_frame()
        scoopFrame = self.scoop_controller.get_frame()
        bucketFrame = self.bucket_controller.get_frame()

        self.get_logger().debug(f"Sending {armFrame}")
        self.get_logger().debug(f"Sending {scoopFrame}")
        self.get_logger().debug(f"Sending {bucketFrame}")
        try:
            self.bus.send(armFrame)
            self.bus.send(scoopFrame)
            self.bus.send(bucketFrame)

        except Exception as e:
            print(e)

    def deadline_callback(self, _info):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.scraper_stop()

    def scraper_stop(self):
        """
        Stops all scraper motors
        """
        self.arm.stop()
        self.scoop.stop()
        self.bucket.stop()

    def update_arm(self, joystick_l: InputJoystick):
        """
        Updates the scraper arm motor
            Left Joystick stick x-axis = direction / velocity
        """
        self.arm.update_velocity(abs(joystick_l.ax_stick_x) * self.velocity_multiplier)
        self.arm.update_direction(self.ARM_BACKWARDS if joystick_l.ax_stick_x >= 0 else self.ARM_FORWARDS)

    def update_scoop(self, joystick_r: InputJoystick):
        """
        Updates the scraper scoop motor
            Right Joystick stick x-axis = direction / velocity
        """
        self.scoop.update_velocity(abs(joystick_r.ax_stick_x) * self.velocity_multiplier)
        self.scoop.update_direction(self.SCOOP_FORWARDS if joystick_r.ax_stick_x >= 0 else self.SCOOP_BACKWARDS)

    def update_bucket(self, joystick_r: InputJoystick):
        """
        Updates the scraper bucket motor
            Left Joystick thumb stick x-axis = direction / velocity
        """
        self.bucket.update_velocity(abs(joystick_r.ax_thumb_x))
        self.bucket.update_direction(self.BUCKET_OPEN if joystick_r.ax_thumb_x >= 0 else self.BUCKET_CLOSE)

    def joystick_l(self, joystick_l: InputJoystick):
        """
        Updates the classes internal msg state
        :param joystick_l: input_interfaces.msg.InputJoystick message from the subscriber callback
        :return: None
        """
        # Update the inputs
        self.update_arm(joystick_l)

    def joystick_r(self, joystick_r: InputJoystick):
        """
        Updates the classes internal msg state
        :param joystick_r: input_interfaces.msg.InputJoystick message from the subscriber callback
        :return: None
        """
        # Update the inputs
        self.velocity_multiplier = abs(joystick_r.ax_slider)

        self.update_scoop(joystick_r)
        self.update_bucket(joystick_r)

def main():
    rclpy.init()
    scraper = ScraperNode()
    rclpy.spin(scraper)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
