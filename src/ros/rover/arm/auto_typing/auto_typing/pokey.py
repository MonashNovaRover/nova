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
from rclpy.action import ActionServer
from rclpy.node import Node
import time
from python_control2 import PythonControl, Controller, Contexts, Interface
from python_control2.hardware_interfaces import QCMDHardware

from nova_interfaces.action import EndEffector

EE_CAN_ID = 0x077   # TODO: update CAN IDs


class EndEffectorController(Controller):
    CAN_BUS = "can1"

    ee_cmd: Interface  # TODO: create EE interface

    def __init__(self, contexts: Contexts):
        super().__init__(contexts)
        self.logger.info("Ready to poke!")

        self.poke_speed = self.declare_parameter("poke_speed", 0.1, "The speed at which the end effector moves")
        self.poke_amount = self.declare_parameter("poke_amount", 1, "How long to poke for")

        self._action_server = ActionServer(
            self,
            EndEffector,
            '/arm/poke',
            self.execute_callback)

    def on_configure(self, command_interfaces, state_interfaces):
        self.ee_cmd = command_interfaces["end_effector/effort"]

    def execute_callback(self, goal_handle):
        end_poke = goal_handle.request.poke

        print(goal_handle.request)
        forward = False

        if end_poke > 0.5:
            forward = True

        self.logger.info(f"Executing end effector goal... poking to {end_poke}")

        for i in range(self.poke_amount):
            self.poke(forward)
            time.sleep(0.25)

        self.ee_cmd.value = 0

        goal_handle.succeed()
        result = EndEffector.Result()
        result.end_poke = end_poke
        return result

    def poke(self, forward: bool):
        if forward:
            self.ee_cmd.value = self.poke_speed
        else:
            self.ee_cmd.value = -self.poke_speed


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
