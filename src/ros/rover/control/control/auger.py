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
from struct import pack
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
    # can bus
    CAN_BUS = "can1"
    # card IDs
    AUGER_ID = 0x063
    DRILL_ID = 0x053
    CARD_ID_RECEIVE = 0x4A2
    # command data
    AUGER_UP = 1
    AUGER_DOWN = -1
    DRILL_CLOCKWISE = 1
    DRILL_COUNTERCLOCKWISE = -1
    # limit switch id
    AUGER_LIMIT_SWITCH_TOP = 0x01
    AUGER_LIMIT_SWITCH_BOTTOM = 0x02
    # limit switch status / data
    AUGER_LIMIT_SWITCH_CLEAR = 0x00
    AUGER_LIMIT_SWITCH_HIT = 0x01



    def __init__(self):
        super().__init__("auger")

        self.get_logger().set_level(logging.DEBUG)
        self.param_can = self.declare_parameter("can_bus", self.CAN_BUS).value
        self.param_auger_velocity_multiplier = self.declare_parameter("auger_velocity_multiplier", 32767/2).value
        self.param_drill_default_velocity = self.declare_parameter("drill_default_velocity", 32767/2).value

        # Initially all motors spin backwards with 0 velocity
        self.auger_direction = self.AUGER_UP
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
        augerFrame = jcan.Frame(self.AUGER_ID, auger_commands)
        drillFrame = jcan.Frame(self.DRILL_ID, drill_commands)

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
        self.get_logger().info(f"Received {hex(frame.id)} {frame.data}")
 
        if frame.id == self.CARD_ID_RECEIVE:
            if frame.data[0] == self.AUGER_LIMIT_SWITCH_TOP:
                if frame.data[1] == self.AUGER_LIMIT_SWITCH_HIT:
                    self.get_logger().info("Top limit switch hit")
                    self.top_limit = True
                else:
                    self.top_limit = False
            elif frame.data[0] == self.AUGER_LIMIT_SWITCH_BOTTOM:
                if frame.data[1] == self.AUGER_LIMIT_SWITCH_HIT:
                    self.get_logger().info("Bottom limit switch hit")
                    self.bottom_limit = True
                else:
                    self.bottom_limit = False
        else:
            self.get_logger().info(f"Received unknown frame {frame}")
        


    def auger_stop_state(self):
        self.auger_velocity = 0
        self.auger_direction = self.AUGER_UP

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
            self.auger_direction = self.AUGER_UP if joystick_r.ax_stick_x >= 0 else self.AUGER_DOWN

            if self.auger_direction == self.AUGER_UP and self.top_limit:
                self.auger_velocity = 0
            elif self.auger_direction == self.AUGER_DOWN and self.bottom_limit:
                self.auger_velocity = 0
            else:
                self.auger_velocity = abs( int( self.param_auger_velocity_multiplier * joystick_r.ax_stick_x ) )

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
        auger_value = self.auger_direction * self.auger_velocity
        drill_value = self.drill_direction * self.drill_velocity
        self.get_logger().info(f"Auger: {auger_value}, Drill: {drill_value}")
        auger_data = list(pack('>h', int(auger_value)))
        drill_data = list(pack('>h', int(drill_value)))

        return auger_data, drill_data


def main():
    rclpy.init()
    auger = AugerNode()
    rclpy.spin(auger)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
