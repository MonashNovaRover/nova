#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: A template ROS node
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: excavation_construction
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
from struct import pack

# example of how to import a custom message type
from core.msg import InputJoystick

# an example of how to import a standard message type
from std_msgs.msg import String
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration


class GimbleCam(Node):

    def __init__(self):
        super().__init__("gimble_cam")

        self.get_logger().set_level(logging.INFO)
        self.param_can = self.declare_parameter("can_bus", "can1").value
        self.param_velocity_factor = self.declare_parameter("velocity_factor", 1.0).value

        # can commands for gimble cam
        self.velocity_cmd = 0x081
        # Initially all motors spin backwards with 0 velocity
        self.x_velocity = 0
        self.y_velocity = 0
        self.joystick_lock = True

        deadline = Duration(nanoseconds=2e8)
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        self.joystick_l_sub = self.create_subscription(InputJoystick, "/control/input_joystick_l",
                                                       self.joystick_l_callback, self.qos, event_callbacks=events)

        self.bus = jcan.Bus()
        self.bus.open(self.param_can)
        self.timer_jcan = self.create_timer(0.05, self.callback_send_can_commands)

    def callback_send_can_commands(self):
        """Take current internal state and publish over CAN
        Sends can commands for scraper scoop, scraper arm and tile placer together
        """
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        cmd_frame = jcan.Frame(self.velocity_cmd, self.get_can_data())
        self.bus.send(cmd_frame)


    def deadline_callback(self, info):
        # Set all speeds to 0
        self.get_logger().warning("200ms Callback deadline missed")
        self.x_velocity = 0
        self.y_velocity = 0

    def joystick_l_callback(self, msg):
        """
        Updates the classes internal msg state
        :return: None
        """
        # Joysticks lock if botton L2 button is pressed on the left joystick
        if msg.btn_bottom_l2_state >= 1 and not self.joystick_lock:
            self.get_logger().info("Joysticks Locked")
            self.joystick_lock = True
        if msg.btn_bottom_l5_state >= 1 and self.joystick_lock:
            self.get_logger().info("Joysticks Unlocked")
            self.joystick_lock = False

        if not self.joystick_lock:
            # Update the inputs
            self.x_velocity = msg.ax_thumb_x
            self.y_velocity = msg.ax_thumb_y
        else:
            self.x_velocity = 0
            self.y_velocity = 0

    def get_can_data(self):
       return list(pack('>bb', int(127*self.param_velocity_factor*self.x_velocity),
                                    int(127*self.param_velocity_factor*self.y_velocity)))


def main():
    rclpy.init()
    excavation_construction = GimbleCam()
    rclpy.spin(excavation_construction)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
