#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the end effector during auto typing.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: EndEffectorController
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - end_effector/effort        [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        auto-typing
AUTHOR(S):      Jackson MacCormick
CREATION:       12/05/26
EDITED:         12/05/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.action import ActionServer, GoalResponse
from rclpy.node import Node
from rclpy.task import Future
import time
from python_control2 import PythonControl, Controller, Contexts, Interface
from python_control2.hardware_interfaces import QCMDHardware

from nova_interfaces.action import EndEffector

EE_CAN_ID = 0x0C1

class EndEffectorController(Controller):
    CAN_BUS = "can1"

    ee_cmd: Interface
    callback_ee_value = 0
    ee_timer = 0
    task_done = None

    def __init__(self, contexts: Contexts):
        super().__init__(contexts)
        self.logger.info("Ready to poke!")

        self.poke_speed = self.declare_parameter("poke_speed", 1, "The speed at which the end effector moves")
        self.poke_amount = self.declare_parameter("poke_amount", 1, "How long to poke for")

        self._action_server = ActionServer(
            self.node,
            EndEffector,
            '/arm/poke',
            self.execute_callback)

    def on_configure(self, command_interfaces, state_interfaces):
        self.ee_cmd = command_interfaces["end_effector/effort"]

    def on_update(self, now, period):
        if self.ee_timer > 0:
            self.ee_timer -= period
            self.ee_cmd.value = self.callback_ee_value
        else:
            if self.task_done and not self.task_done.done():
                self.task_done.set_result(True)
            self.ee_cmd.value = 0

    async def execute_callback(self, goal_handle):
        end_poke = goal_handle.request.poke

        forward = end_poke > 0.5
        self.ee_timer = self.poke_amount.value

        self.logger.info(f"Executing end effector goal... poking to {end_poke}")

        self.poke(forward)

        self.task_done = Future()

        await self.task_done

        goal_handle.succeed()

        result = EndEffector.Result()
        result.end_poke = end_poke
        return result

    def poke(self, forward: bool):
        if forward:
            self.callback_ee_value = self.poke_speed.value
        else:
            self.callback_ee_value = -1 * self.poke_speed.value


def main():
    rclpy.init()
    node = Node("pokey")

    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller("controller", EndEffectorController) \
        .with_hardware("end_effector", QCMDHardware, can_id=EE_CAN_ID) \
        .with_jcan() \
        .spin()


if __name__ == "__main__":
    main()
