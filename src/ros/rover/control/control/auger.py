#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: A template ROS node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: auger
TOPICS:
  - subscriber: /control/input_joystick_l [InputJoystick]
  - subscriber: /control/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Tristan Clark
CREATION:	24/02/2024
EDITED:		24/02/2024
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


class AugerNode(Node):
    # card IDs
    CARD_ID_SEND = 0x0C0
    CARD_ID_RECEIVE = 0x4C0
    # command data
    AUGER_ID_UP = 0x1
    AUGER_ID_DOWN = 0x2
    DRILL_CLOCKWISE = 0x3
    DRILL_COUNTERCLOCKWISE = 0x4
    # limit switch data
    AUGER_LIMIT_SWITCH_TOP_CLEAR = 0x0100
    AUGER_LIMIT_SWITCH_TOP_HIT = 0x0101
    AUGER_LIMIT_SWITCH_BOTTOM_CLEAR = 0x0200
    AUGER_LIMIT_SWITCH_BOTTOM_HIT = 0x0201


    def __init__(self):
        super().__init__("auger")

        self.get_logger().set_level(logging.WARN)
        self.param_can = self.declare_parameter("can_bus", "can0").value
        self.param_auger_velocity_multiplier = self.declare_parameter("scraper_arm_multiplier", 255).value
        self.param_drill_default_velocity = self.declare_parameter("scraper_scoop_multiplier", 255).value

        # Initially all motors spin backwards with 0 velocity
        self.auger_direction = self.AUGER_ID_UP
        self.drill_direction = self.DRILL_CLOCKWISE
        self.auger_velocity = 0
        self.drill_velocity = 0
        
        self.top_limit = False
        self.bottom_limit = False
        self.joystick_lock = True


        deadline = Duration(nanoseconds=2e8)        
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        self.joystick_l_sub = self.create_subscription(InputJoystick, "/control/input_joystick_l", self.joystick_l_callback, self.qos, event_callbacks=events)
        self.joystick_r_sub = self.create_subscription(InputJoystick, "/control/input_joystick_r", self.joystick_r_callback, self.qos, event_callbacks=events)

        self.bus = jcan.Bus()
        self.bus.set_id_filter_mask(self.CARD_ID_RECEIVE, 0xFFF)

        self.bus.add_callback(self.CARD_ID_RECEIVE, self.callback_receive_can_feedback)

        self.bus.open(self.param_can)
        self.timer_jcan_commands = self.create_timer(0.05, self.callback_send_can_commands)
        self.timer_jcan_spin = self.create_timer(0.01, self.bus.spin)


    def callback_send_can_commands(self):
        """Take current internal state and publish over CAN
        Sends can commands for auger and drill together
        """
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        auger_commands, drill_commands = self.get_auger_commands()
        augerFrame = jcan.Frame(self.CARD_ID_SEND, auger_commands)
        drillFrame = jcan.Frame(self.CARD_ID_SEND, drill_commands)

        self.get_logger().info(f"Sending {augerFrame}")
        self.get_logger().info(f"Sending {drillFrame}")
        try:
            self.bus.send(augerFrame)
            self.bus.send(drillFrame)

        except Exception as e:
            print(e)

    def callback_receive_can_feedback(self, frame: jcan.Frame):
        """Receive can feedback for auger limit switches
        """
        self.get_logger().info(f"Received {frame}")

        if frame.id == self.CARD_ID_RECEIVE:
            if frame.data[0] == self.AUGER_LIMIT_SWITCH_TOP_HIT:
                self.get_logger().info("Top limit switch hit")
                self.top_limit = True
            elif frame.data[0] == self.AUGER_LIMIT_SWITCH_TOP_CLEAR:
                self.top_limit = False

            if frame.data[0] == self.AUGER_LIMIT_SWITCH_BOTTOM_HIT:
                self.get_logger().info("Bottom limit switch hit")
                self.bottom_limit = True
            elif frame.data[0] == self.AUGER_LIMIT_SWITCH_BOTTOM_CLEAR:
                self.bottom_limit = False

        else:
            self.get_logger().info(f"Received unknown frame {frame}")
        


    def auger_stop_state(self):
        self.auger_velocity = 0
        self.auger_direction = self.AUGER_ID_UP

    def drill_stop_state(self):
        self.drill_velocity = 0
        self.drill_direction = self.DRILL_CLOCKWISE

    
    def deadline_callback(self, _info):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.auger_stop_state()
        self.drill_stop_state()


    def joystick_l_callback(self, msg: InputJoystick):
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
            self.joystick_lock = False



    def joystick_r_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        self.get_logger().info("called r")

        joystick_r = msg

        if not self.joystick_lock:
            # Update the inputs
            self.tile_placer_velocity = abs( int( self.param_auger_velocity_multiplier * joystick_r.ax_stick_x ) )
            self.tile_placer_direction = self.AUGER_ID_UP if joystick_r.ax_stick_x >= 0 else self.AUGER_ID_DOWN
            
            if joystick_r.btn_thumb_r_state >= 1:
                self.drill_direction = self.DRILL_CLOCKWISE
            elif joystick_r.btn_thumb_l_state >= 1:
                self.drill_direction = self.DRILL_COUNTERCLOCKWISE
            
            if joystick_r.btn_thumb_u_state >= 1:
                self.drill_velocity = self.param_drill_default_velocity
            else:
                self.drill_velocity = 0          
        else:
            # if joysticks locked
            self.get_logger().info("joysticks locked!")
            self.auger_stop_state()
            self.drill_stop_state()



    def get_auger_commands(self):
        auger_data = [self.auger_direction, self.auger_velocity]
        drill_data = [self.drill_direction, self.drill_velocity]

        return auger_data, drill_data


def main():
    rclpy.init()
    auger = AugerNode()
    rclpy.spin(auger)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
