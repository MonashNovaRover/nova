#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Tile Placer using Joysticks
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: tile_placer
TOPICS:
  - subscriber: /inputs/input_joystick_l [InputJoystick]
  - subscriber: /inputs/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: control
COMMAND: ros2 run control tile_placer.py
RUN ON: Rover
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUTHOR(S):	Tristan Clark, Felicity Matthews, Brandon Chung
CREATION:	02/02/2024
EDITED:		16/03/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""
from python_control.controls.Direction import Direction
from python_control.controllers.Card import Card
from python_control.controls.OneAxisVelocityControl import OneAxisVelocityControl
from python_control.controllers.CMDVelocityController import CMDVelocityController
from python_control.JoystickControllerNode import JoystickControllerNode
import rclpy

# import the joystick ROS message we are listening to
from input_interfaces.msg import InputJoystick

CARD = Card.CMD

class TilePlacerNode(JoystickControllerNode):
    # can bus
    CAN_BUS = "can0"
    # card IDs
    # only used when card = CMD, works with QCMDs
    # 0x0C? ports can handle slightly higher current on QCMDs, 0x0D? ports are slightly weaker
    CMD_ID_TILE_PLACER = 0x0D2
    # only used when card = JONO
    JONO_ID_TILE_PLACER = 0x0A0
    # jono commands
    TILE_PLACER_ID_UP = 0x02
    TILE_PLACER_ID_DOWN = 0x01

    # directions
    TILE_PLACER_UP = Direction.POSITIVE
    TILE_PLACER_DOWN = Direction.NEGATIVE

    # max_velocity percent
    TILE_PLACER_MAX_VELOCITY = 0.70

    # how long to twitch tile placer for
    TWITCH_PERIOD = 20 # To be tested with tile placer

    # ROS param names
    CAN_BUS_PARAM = "can_bus"
    TILE_PLACER_MAX_VEL_PERCENT_PARAM = "tile_placer_max_vel_percent"

    def __init__(self):
        super().__init__("tile_placer", can_bus=self.CAN_BUS)
        logger = self.get_logger()

        self.declare_parameter(self.TILE_PLACER_MAX_VEL_PERCENT_PARAM, self.TILE_PLACER_MAX_VELOCITY)

        self.tile_placer = OneAxisVelocityControl(
            logger=logger,
            max_percent=self.get_parameter(self.TILE_PLACER_MAX_VEL_PERCENT_PARAM).value,
            direction=self.TILE_PLACER_UP,
        )

        self.tile_placer_controller = CMDVelocityController(
            logger=logger,
            bus=self.bus,
            frame_id=self.CMD_ID_TILE_PLACER,
            control=self.tile_placer,
        )

        self.velocity_multiplier = 0.0

        # check if tile placer is moving
        self.twitching = 0

        self.timer_jcan = self.create_timer(0.05, self.callback_send_can_commands)
        self.start_can()

        self.get_logger().info(f"Tile Placer started on {self.get_parameter(self.CAN_BUS_PARAM).value}")

    def callback_send_can_commands(self):
        """Take current internal state and publish over CAN
        Sends can commands for tile placer
        """
        # stop tile placer if twitching finished
        if self.twitching >= self.TWITCH_PERIOD:
            self.twitching = -1
            self.tile_placer_stop()

        self.twitching += 1

        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        tilePlacerFrame = self.tile_placer_controller.get_frame()

        self.get_logger().debug(f"Sending {tilePlacerFrame}")
        try:
            self.bus.send(tilePlacerFrame)

        except Exception as e:
            print(e)
    
    def deadline_callback(self, info: InputJoystick):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.tile_placer_stop()

    def tile_placer_stop(self):
        self.tile_placer.stop()
        self.twitching = 0

    def update_tile_placer(self, joystick_r: InputJoystick):
        # State 1 is when button is first pressed
        if joystick_r.btn_thumb_l_state == 1 and self.twitching <= 0:
            self.get_logger().debug("Tile Placer UP")
            self.tile_placer.update_direction(self.TILE_PLACER_UP)
            self.tile_placer.update_velocity(self.velocity_multiplier)
            self.twitching += 1
        elif joystick_r.btn_thumb_r_state == 1 and self.twitching <= 0:
            self.get_logger().debug("Tile Placer DOWN")
            self.tile_placer.update_direction(self.TILE_PLACER_DOWN)
            self.tile_placer.update_velocity(self.velocity_multiplier)
            self.twitching += 1
        elif self.twitching > 0:
            pass
        else:
            self.tile_placer_stop()

    def joystick_l(self, joystick_l: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.velocity_multiplier = abs(joystick_l.ax_slider)

    def joystick_r(self, joystick_r: InputJoystick):
        """
        Updates the classes internal msg state
        :param joystick_r: input_interfaces.msg.RoverPose message from the subscriber callback
        :return: None
        """
        # Update inputs
        self.update_tile_placer(joystick_r)

def main():
    rclpy.init()
    tile_placer = TilePlacerNode()
    rclpy.spin(tile_placer)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
