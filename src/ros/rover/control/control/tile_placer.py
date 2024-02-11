#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: A template ROS node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: tile_placer
TOPICS:
  - subscriber: /control/input_joystick_l [InputJoystick]
  - subscriber: /control/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Manika Goyal, Max Tory, Taaj Street
CREATION:	08/03/2023
EDITED:		18/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""
import rclpy
from rclpy.node import Node
import jcan, logging

# example of how to import a custom message type
from core.msg import InputJoystick

# an example of how to import a standard message type
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration


class TilePlacerNode(Node):
    # set motor ids
    TILE_PLACER_ID_FORWARDS = 0x2
    TILE_PLACER_ID_BACKWARDS = 0x1

    def __init__(self):
        super().__init__("tile_placer")

        self.get_logger().set_level(logging.WARN)
        self.param_can = self.declare_parameter("can_bus", "can0").value
        self.param_tile_placer_multiplier = self.declare_parameter("tile_placer_multiplier", 200).value

        self.tile_placer_activated = False

        # Initially all motors spin backwards with 0 velocity
        self.tile_placer_direction = self.TILE_PLACER_ID_BACKWARDS
        self.tile_placer_velocity = 0

        self.joystick_lock = True


        deadline = Duration(nanoseconds=2e8)        
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        self.joystick_l_sub = self.create_subscription(InputJoystick, "/control/input_joystick_l", self.joystick_l_callback, self.qos, event_callbacks=events)
        self.joystick_r_sub = self.create_subscription(InputJoystick, "/control/input_joystick_r", self.joystick_r_callback, self.qos, event_callbacks=events)

        self.bus = jcan.Bus()
        self.bus.open(self.param_can)
        self.timer_jcan = self.create_timer(0.05, self.callback_send_can_commands)


    def callback_send_can_commands(self):
        """Take current internal state and publish over CAN
        Sends can commands for tile placer
        """
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        tile_placer_commands = self.get_tile_placer_can_commands()
        tilePlacerFrame = jcan.Frame(0x0A0, tile_placer_commands)

        self.get_logger().info(f"Sending {tilePlacerFrame}")
        try:
            self.bus.send(tilePlacerFrame)

        except Exception as e:
            print(e)

    
    def deadline_callback(self, info: InputJoystick):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.tile_placer_velocity = 0
        self.tile_placer_direction = self.TILE_PLACER_ID_FORWARDS


    def joystick_l_callback(self, msg):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.get_logger().info("called l")

        joystick_l = msg

        # Joysticks lock if botton L2 button is pressed on the left joystick
        if joystick_l.btn_bottom_l2_state >= 1 and not self.joystick_lock:
            self.get_logger().info("Joysticks Locked")
            self.joystick_lock = True

        # joysticks are only unlocked when bottom l5 button is pressed on the left joystick
        if joystick_l.btn_bottom_l5_state >= 1 and self.joystick_lock:
            self.get_logger().info("Joysticks Unlocked")
            if self.tile_placer_activated:
                self.get_logger().info("Tile Placer ON")
            else:
                self.get_logger().info("Tile Placer OFF")
            self.joystick_lock = False


    def joystick_r_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        self.get_logger().info("called r")

        joystick_r = msg

        # Tile Placer mode is off
        if joystick_r.btn_bottom_r1_state >= 1 and self.tile_placer_activated:
            self.get_logger().info("Tile Placer OFF")
            self.tile_placer_activated = False

        # tile placer is unlocked when bottom r4 button is pressed on the right joystick
        # Tile Placer mode is on
        if joystick_r.btn_bottom_r4_state >= 1 and not self.tile_placer_activated:
            self.get_logger().info("Tile Placer ON")
            self.tile_placer_activated = True

        if not self.joystick_lock and self.tile_placer_activated:
            # Update the inputs
            self.tile_placer_velocity = abs( int( self.param_tile_placer_multiplier * joystick_r.ax_stick_x ) )
            self.tile_placer_direction = self.TILE_PLACER_ID_FORWARDS if joystick_r.ax_stick_x >= 0 else self.TILE_PLACER_ID_BACKWARDS

        elif not self.joystick_lock and not self.tile_placer_activated:

            # set tile placer velocities to 0
            self.tile_placer_velocity = 0
            self.tile_placer_direction = self.TILE_PLACER_ID_FORWARDS
        else:
            # if joysticks locked
            self.get_logger().info("joysticks locked!")
            self.tile_placer_activated = False

            # set tile placer velocity to 0
            self.tile_placer_velocity = 0
            self.tile_placer_direction = self.TILE_PLACER_ID_FORWARDS


    def get_tile_placer_can_commands(self):
        tile_placer_data = []
        tile_placer_data.append(self.tile_placer_direction)        
        tile_placer_data.append(self.tile_placer_velocity)

        return tile_placer_data


def main():
    rclpy.init()
    tile_placer = TilePlacerNode()
    rclpy.spin(tile_placer)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
