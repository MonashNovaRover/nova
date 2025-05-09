#!/usr/bin/env python3
# Purpose: Autonomous typing

"""
Sources for Old CAN Commands: 
- /home/nova/nova/src/other/libcanmd/src/cmd.cpp
- /home/nova/nova/src/ros/rover/arm/arm/src/arm_driver/arm_driver.cpp

IDs for both motors will be two of the following (on QCMD now): 0C1, 0C2, 0D1, 0D2 but data sent will be the same
"""

import rclpy
from rclpy.action import ActionServer
from rclpy.node import Node
import jcan
from enum import Enum

from nova_interfaces.action import EndEffector
from inputs.input_interfaces.msg import InputJoystick


class EndEffectorLinearActuationMode(Enum):
    STOP = 0
    BACKWARDS = 1
    FORWARDS = 2


class EndEffectorActionServer(Node):
    
    END_EFFECTOR_DRIVE_CAN_ID = 0x0C1
    END_EFFECTOR_LINEAR_ACTUATION_CAN_ID = 0x0C2
    SCALING_FACTOR = 0.5    # THIS WAS USED FOR CERTAIN ARM CONFIGURATIONS SO I LEFT IT IN, used to halve voltage so that 12V instead of 24V

    def __init__(self):
        super().__init__('arm_end_effector_action_server')
        self.get_logger().set_level(logging.INFO)
        self.get_logger().info("Arm End Effector Action Server starting")

        self.declare_parameter("can_bus", "can1")
        self.declare_parameter("end_effector_drive_CAN_ID", EndEffectorActionServer.END_EFFECTOR_DRIVE_CAN_ID)
        self.declare_parameter("end_effector_linear_actuation_CAN_ID", EndEffectorActionServer.END_EFFECTOR_LINEAR_ACTUATION_CAN_ID)

        self._action_server = ActionServer(
            self,
            EndEffector,
            'arm/end_effector/action',
            self.execute_callback)

        self.left_joystick_sub = self.create_subscription(
            InputJoystick,
            "/inputs/input_joystick_l",
            self.left_joystick_callback,
            10)

        self.right_joystick_sub = self.create_subscription(
            InputJoystick,
            "/inputs/input_joystick_r",
            self.right_joystick_callback,
            10)

        # initial state
        self.position = 0   # 0 is fully retracted, 1 is fully extended
        self.is_extending = False
        self.end_effector_drive = 0
        self.linear_actuation_mode = 0
        self.is_locked = True

        # for CAN commands
        self.bus = jcan.Bus()
        self.bus.open(self.get_parameter(self.CAN_BUS_PARAM).value)
        self.timer_spin_can = self.create_timer(0.01, self.bus.spin)
        self.timer_send_can_commands = self.create_timer(0.01, self.send_can_commands)
        # add can callback for updating position so that self.position can be assumed as always accurate

    def left_joystick_callback(self, msg):
        if msg.btn_bottom_l2_state == 1:
            self.is_locked = True
        if msg.btn_bottom_l5_state == 1:
            self.is_locked = False

        if not self.is_locked:
            if msg.ax_thumb_x == 0:
                self.linear_actuation_mode = EndEffectorLinearActuationMode.STOP
            elif msg.ax_thumb_x > 0:
                self.linear_actuation_mode = EndEffectorLinearActuationMode.FORWARDS
            else:
                self.linear_actuation_mode = EndEffectorLinearActuationMode.BACKWARDS
        else:
            self.linear_actuation_mode = EndEffectorLinearActuationMode.STOP

    def right_joystick_callback(self, msg):
        if not self.is_locked:
            self.end_effector_drive = msg.ax_thumb_x
        else:
            self.end_effector_drive = 0

    def send_can_commands(self):
        self.set_linear_actuator(self.linear_actuation_mode)
        self.drive_end_effector(self.end_effector_drive)

    def execute_callback(self, goal_handle):
        end_poke = goal_handle.request.poke
        self.get_logger().info(f"Executing end effector goal... poking to {end_poke}")
        feedback_msg = EndEffector.Feedback()

        # Extending to poke position
        self.poke_to(end_poke)

        while self.position < end_poke:
            self.bus.spin()
            if self.poke_position != feedback_msg.current_poke:
                feedback_msg.current_poke = self.poke_position
                feedback_msg.is_extending = self.is_extending
                goal_handle.publish_feedback(feedback_msg)
            self.poke_to(end_poke)

        self.stop_poke()

        goal_handle.succeed()
        result = EndEffect.Result()
        result.end_poke = self.poke_position
        return result

    def poke_to(self, end_poke: float):
        if end_poke > self.position:
            self.set_linear_actuator(EndEffectorLinearActuationMode.FORWARDS)
        elif end_poke < self.position:
            self.set_linear_actuator(EndEffectorLinearActuationMode.BACKWARDS)
        if end_poke == self.position:
            self.set_linear_actuator(EndEffectorLinearActuationMode.STOP)

    def stop_poke(self):
        self.set_linear_actuator(EndEffectorLinearActuationMode.STOP)

    def drive_end_effector(self, value: float):
        """
        value: float between -1 and 1
        """
        scaled_value = 32767.0 * value * EndEffectorActionServer.SCALING_FACTOR
        frame = jcan.Frame(self.get_parameter("end_effector_drive_CAN_ID").value, [scaled_value >> 8, scaled_value & 0xFF])
        self.bus.send(frame)

    def set_linear_actuator(self, mode: EndEffectorLinearActuationMode):
        """
        mode: determines direction of the linear actuation
        """
        frame = jcan.Frame(self.get_parameter("end_effector_linear_actuation_CAN_ID").value, [mode])
        self.bus.send(frame)


def main():
    rclpy.init()
    node = EndEffectorActionServer()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()