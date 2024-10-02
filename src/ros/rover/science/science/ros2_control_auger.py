#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control Auger Height and Drill Spin using Joysticks
Translated from Tristan's original version of this code.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: auger
TOPICS:
  - subscriber: /inputs/input_joystick_l [InputJoystick]
  - subscriber: /inputs/input_joystick_r [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Abby Ahmed
CREATION:	02/10/2024
EDITED:		xx/xx/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

"""
import rclpy, jcan, logging
from struct import pack
from rclpy.node import Node
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration

# import the joystick ROS message we are listening to
from input_interfaces.msg import InputJoystick


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
    AUGER_LIMIT_SWITCH_HIT = 0xFF
    # max_velocity
    MAX_VELOCITY = 32767 * (3/4) # 3/4 of max possible value sent to motor
    # ROS parameter names
    CAN_BUS_PARAM = "can_bus"
    AUGER_MAX_VELOCITY_PARAM = "auger_max_vel"
    DRILL_MAX_VELOCITY_PARAM = "drill_max_vel"

    def __init__(self):
        super().__init__("auger")

        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Auger starting")

        self.declare_parameter(self.CAN_BUS_PARAM, self.CAN_BUS)
        self.declare_parameter(self.AUGER_MAX_VELOCITY_PARAM, self.MAX_VELOCITY)
        self.declare_parameter(self.DRILL_MAX_VELOCITY_PARAM, self.MAX_VELOCITY)

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

        self.joystick_l_sub = self.create_subscription(InputJoystick, "/inputs/input_joystick_l", self.joystick_l_callback, self.qos, event_callbacks=events)
        self.joystick_r_sub = self.create_subscription(InputJoystick, "/inputs/input_joystick_r", self.joystick_r_callback, self.qos, event_callbacks=events)

        self.bus = jcan.Bus()
        self.bus.set_id_filter_mask(self.CARD_ID_RECEIVE, 0xFFF)

        self.bus.add_callback(self.CARD_ID_RECEIVE, self.callback_receive_can_feedback)

        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)
        self.timer_jcan_commands = self.create_timer(0.05, self.callback_send_can_commands)
        self.timer_jcan_spin = self.create_timer(0.01, self.bus.spin)

        self.get_logger().info(f"Auger started on {self.get_parameter(self.CAN_BUS_PARAM).value}")
        self.get_logger().info("Joysticks Locked")



    def callback_send_can_commands(self):
        """Take current internal state and publish over CAN
        Sends can commands for auger and drill together
        """
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        auger_commands, drill_commands = self.get_can_commands()
        augerFrame = jcan.Frame(self.AUGER_ID, auger_commands)
        drillFrame = jcan.Frame(self.DRILL_ID, drill_commands)

        self.get_logger().debug(f"Sending {augerFrame}")
        self.get_logger().debug(f"Sending {drillFrame}")
        try:
            self.bus.send(augerFrame)
            self.bus.send(drillFrame)

        except Exception as e:
            print(e)

    def callback_receive_can_feedback(self, frame: jcan.Frame):
        """Receive can feedback for auger limit switches
        """
        self.get_logger().debug(f"Received {hex(frame.id)} {frame.data}")
 
        if frame.id == self.CARD_ID_RECEIVE:
            if frame.data[0] == self.AUGER_LIMIT_SWITCH_TOP:
                if frame.data[1] == self.AUGER_LIMIT_SWITCH_CLEAR:
                    self.top_limit = False
                else:
                    self.get_logger().debug("Top limit switch hit")
                    self.top_limit = True
               
            elif frame.data[0] == self.AUGER_LIMIT_SWITCH_BOTTOM:
                if frame.data[1] == self.AUGER_LIMIT_SWITCH_CLEAR:
                    self.bottom_limit = False
                else:
                    self.get_logger().debug("Bottom limit switch hit")
                    self.bottom_limit = True
        else:
            self.get_logger().warn(f"Received unknown frame {frame}")
        


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

    def check_joystick_lock(self):
        if self.joystick_lock:
            self.auger_stop_state()
            self.drill_stop_state()
            return True
        return False

    def update_joystick_lock(self, joystick_l: InputJoystick):
        # Joysticks lock if botton L2 button is pressed on the left joystick
        if joystick_l.btn_bottom_l2_state >= 1 and not self.joystick_lock:
            self.get_logger().info("Joysticks Locked")
            self.joystick_lock = True

        # Joysticks unlocked when bottom l5 button is pressed on the left joystick
        if joystick_l.btn_bottom_l5_state >= 1 and self.joystick_lock:
            self.get_logger().info("Joysticks Unlocked")
            self.joystick_lock = False

    def update_auger_height(self, joystick_r: InputJoystick):
        # Auger height direction is determined by the right joystick's x-axis direction
        self.auger_direction = self.AUGER_DOWN if joystick_r.ax_stick_x >= 0 else self.AUGER_UP 

        # Auger velocity is determined by the right joystick's x-axis magnitude
        # If the auger is at the top or bottom limit, the velocity is set to 0
        if ((self.auger_direction == self.AUGER_UP and self.top_limit) or 
            (self.auger_direction == self.AUGER_DOWN and self.bottom_limit)):
            self.auger_velocity = 0
        else:
            self.auger_velocity = abs(int(self.get_parameter(self.AUGER_MAX_VELOCITY_PARAM).value * joystick_r.ax_stick_x))

    def update_drill_spin(self, joystick_r: InputJoystick):
        # Drill spin direction is determined by the right joystick thumb buttons
        # Thumb right = clockwise, Thumb left = counterclockwise
        if joystick_r.btn_thumb_r_state >= 1:
            self.drill_direction = self.DRILL_CLOCKWISE
        elif joystick_r.btn_thumb_l_state >= 1:
            self.drill_direction = self.DRILL_COUNTERCLOCKWISE
        
        # Drill spin velocity is determined by the right joystick trigger
        if joystick_r.btn_thumb_u_state >= 1:
            self.drill_velocity = self.get_parameter(self.DRILL_MAX_VELOCITY_PARAM).value
        else:
            self.drill_velocity = 0      

    def joystick_l_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :return: None
        """
        self.get_logger().debug("called l")

        joystick_l = msg

        self.update_joystick_lock(joystick_l)

    



    def joystick_r_callback(self, msg: InputJoystick):
        """
        Updates the classes internal msg state
        :param msg: core.msg.RoverPose message from the subscriber callback
        :return: None
        """
        self.get_logger().debug("called r")

        joystick_r = msg

        if self.check_joystick_lock():
            return

        # Update the inputs

        self.update_auger_height(joystick_r)
        self.update_drill_spin(joystick_r)

          


    # construct the can commands for the auger and drill
    def get_can_commands(self):
        auger_data_value = self.auger_direction * self.auger_velocity
        drill_data_value = self.drill_direction * self.drill_velocity
        self.get_logger().debug(f"Auger: {auger_data_value}, Drill: {drill_data_value}")

        # pack the values into a list of bytes
        # packing int into 2 bytes big endian
        auger_data = list(pack('>h', int(auger_data_value)))
        drill_data = list(pack('>h', int(drill_data_value)))

        return auger_data, drill_data


def main():
    rclpy.init()
    auger = AugerNode()
    rclpy.spin(auger)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
