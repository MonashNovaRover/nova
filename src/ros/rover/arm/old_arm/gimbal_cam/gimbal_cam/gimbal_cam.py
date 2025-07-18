#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: ROS Node for controlling the gimbal cam
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: gimbal_cam
TOPICS:
  - subscriber: /inputs/input_joystick_l [InputJoystick]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Taaj Street
CREATION:	08/05/2023
EDITED:		27/05/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Q: Does locking the other device matter here? Both are simultaneously working. 
"""
import rclpy
from rclpy.node import Node
import jcan, logging
from struct import pack

# example of how to import a custom message type
from input_interfaces.msg import InputJoystick
from input_interfaces.msg import InputKeyboard

# an example of how to import a standard message type
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from rclpy.subscription import SubscriptionEventCallbacks
from rclpy.duration import Duration


class GimbalCam(Node):
    CHASSIS_CAM = 0
    ARM_CAM = 1
    JOYSTICK = 0
    KEYBOARD = 1
    def __init__(self):
        super().__init__("gimbal_cam")

        self.get_logger().set_level(logging.INFO)
        self.declare_parameter("chassis_cam", False)
        self.param_do_pwm = self.declare_parameter("do_pwm", True).value
        self.param_velocity_steps = self.declare_parameter("velocity_steps", 10).value
        self.max_velocity_cmd = self.declare_parameter("max_velocity_cmd", 127).value
        self.min_velocity_cmd = 0x3F if self.param_do_pwm else self.declare_parameter("min_velocity", 0.1).value * \
                                                           self.max_velocity_cmd
        self.velocity_increment = 1/self.param_velocity_steps
        # can commands for gimbal cam
        self.arm_velocity_cmd = 0x081
        self.chassis_velocity_cmd = 0x0A1
        self.velocity = 0.5
        self.x_velocity = 0
        self.y_velocity = 0
        self.device_choice = self.KEYBOARD

        deadline = Duration(nanoseconds=2e8)
        events = SubscriptionEventCallbacks(deadline=self.deadline_callback)
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        # Subscriptions
        self.joystick_l_sub = self.create_subscription(InputJoystick, "/inputs/input_joystick_l",
                                                       self.joystick_l_callback, self.qos, event_callbacks=events)
        self.keyboard_sub = self.create_subscription(InputKeyboard, "/inputs/input_keyboard",
                                                     self.keyboard_callback, self.qos, event_callbacks=events)

        self.joystick_connected = False
        # CAN buses
        self.cam_select = self.ARM_CAM # default to arm, as chassis cam may be disabled
        if self.get_parameter('chassis_cam').value:    
            self.chassis_cam = jcan.Bus()
            self.chassis_cam.open('can0')
        self.arm_cam = jcan.Bus()
        self.arm_cam.open("can1")
        self.timer_jcan = self.create_timer(0.05, self.callback_send_can_commands)
        self.get_logger().info("Gimbal Cam Started")


    def callback_send_can_commands(self):
        """Take current internal state and publish over CAN
        Sends can commands for scraper scoop, scraper arm and tile placer together
        """
        # The list of values will be cast to uint8's by JCAN library - so be careful to double check the values!
        cmd = self.arm_velocity_cmd if self.cam_select == self.ARM_CAM else self.chassis_velocity_cmd
        cmd_frame = jcan.Frame(cmd, self.get_can_data())
        if self.cam_select == self.CHASSIS_CAM:
            self.chassis_cam.send(cmd_frame)
        else:
            self.arm_cam.send(cmd_frame)

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
        joystick_l = msg
        self.joystick_connected = joystick_l.connected
        # Joysticks lock if botton L2 button is pressed on the left joystick
        if self.device_choice != self.KEYBOARD and joystick_l.btn_bottom_r2_state >= 1:
            self.get_logger().info("Swapped to Keyboard Control")
            self.device_choice = self.KEYBOARD

        if self.device_choice == self.JOYSTICK:
            if self.get_parameter('chassis_cam').value:
                if joystick_l.btn_bottom_l3_state == 1:
                    self.cam_select = self.CHASSIS_CAM
                    self.get_logger().info("Chassis Camera Selected")
                elif joystick_l.btn_bottom_l6_state == 1:
                    self.cam_select = self.ARM_CAM
                    self.get_logger().info("Arm Camera Selected")
            elif joystick_l.btn_bottom_l3_state == 1:
                self.get_logger().info("Cannot switch, Chassis Camera disabled")
            #set the velocity factor
            if joystick_l.btn_bottom_r1_state == 1:
                self.velocity = max(self.velocity - self.velocity_increment, 0)
                self.get_logger().info(f"Velocity decreased to {self.velocity}")
            elif joystick_l.btn_bottom_r3_state == 1:
                self.velocity = min(self.velocity + self.velocity_increment, 1)
                self.get_logger().info(f"Velocity increased to {self.velocity}")

            # set the y velocity
            if joystick_l.btn_bottom_r2_state >= 1:
                self.y_velocity = -self.get_velocity_cmd()
            elif joystick_l.btn_bottom_r5_state >= 1:
                self.y_velocity = self.get_velocity_cmd()
            else:
                self.y_velocity = 0

            # set the x velocity
            if joystick_l.btn_bottom_r4_state >= 1:
                self.x_velocity = -self.get_velocity_cmd()
            elif joystick_l.btn_bottom_r6_state >= 1:
                self.x_velocity = self.get_velocity_cmd()
            else:
                self.x_velocity = 0

        else:
            self.x_velocity = 0
            self.y_velocity = 0

    def keyboard_callback(self, msg):
        SDL_SCANCODE_RIGHT = 79
        SDL_SCANCODE_LEFT = 80
        SDL_SCANCODE_DOWN = 81
        SDL_SCANCODE_UP = 82
        SDL_SCANCODE_1 = 30
        SDL_SCANCODE_0 = 39
        
        keyboard = msg
        # toggle the lock with ctrl(L), and swap to joystick control with ctrl(0)
        if GimbalCam.ctrl(GimbalCam.shift(SDL_SCANCODE_0)) in keyboard.keys_pressed:
            if self.device_choice == self.KEYBOARD and self.joystick_connected:
                self.get_logger().info("Swapped to Joystick Control")
                self.device_choice = self.JOYSTICK
            elif not self.joystick_connected:
                self.get_logger().info("Cannot switch, Joystick not connected")
                
        if self.device_choice == self.KEYBOARD:
            if self.get_parameter('chassis_cam').value:
                # Change between the cameras using alt(0) and alt(1)
                if GimbalCam.alt(SDL_SCANCODE_0) in keyboard.keys_pressed:
                    self.cam_select = self.CHASSIS_CAM
                    self.get_logger().info("Chassis Camera Selected")
                elif GimbalCam.alt(SDL_SCANCODE_1) in keyboard.keys_pressed:
                    self.cam_select = self.ARM_CAM
                    self.get_logger().info("Arm Camera Selected")
            elif GimbalCam.alt(SDL_SCANCODE_0) in keyboard.keys_pressed:
                self.get_logger().info("Cannot switch, Chassis Camera disabled")
            #set the velocity factor
            if GimbalCam.ctrl(SDL_SCANCODE_DOWN) in keyboard.keys_pressed:
                self.velocity = max(self.velocity - self.velocity_increment, 0)
                self.get_logger().info(f"Velocity decreased to {self.velocity}")
            elif GimbalCam.ctrl(SDL_SCANCODE_UP) in keyboard.keys_pressed:
                self.velocity = min(self.velocity + self.velocity_increment, 1)
                self.get_logger().info(f"Velocity increased to {self.velocity}")

            # set the y velocity
            if SDL_SCANCODE_UP in keyboard.keys_pressed or SDL_SCANCODE_UP in keyboard.keys_repeated:
                self.y_velocity = -self.get_velocity_cmd()
            elif SDL_SCANCODE_DOWN in keyboard.keys_pressed or SDL_SCANCODE_DOWN in keyboard.keys_repeated:
                self.y_velocity = self.get_velocity_cmd()
            else:
                self.y_velocity = 0

            # set the x velocity
            if SDL_SCANCODE_LEFT in keyboard.keys_pressed or SDL_SCANCODE_LEFT in keyboard.keys_repeated:
                self.x_velocity = -self.get_velocity_cmd()
            elif SDL_SCANCODE_RIGHT in keyboard.keys_pressed or SDL_SCANCODE_RIGHT in keyboard.keys_repeated:
                self.x_velocity = self.get_velocity_cmd()
            else:
                self.x_velocity = 0

        else:
            self.x_velocity = 0
            self.y_velocity = 0

    @staticmethod
    def ctrl(num):
        return num | (1<<31)
        
    @staticmethod
    def shift(num):
        return num | (1<<30)
    
    @staticmethod
    def alt(num):
        return num | (1<<29)


    def get_velocity_cmd(self):
        return self.velocity*(self.max_velocity_cmd - self.min_velocity_cmd) + self.min_velocity_cmd
    def get_can_data(self):
       return list(pack('>bb', int(self.x_velocity), int(self.y_velocity)))


def main():
    rclpy.init()
    gimbal_cam = GimbalCam()
    rclpy.spin(gimbal_cam)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
