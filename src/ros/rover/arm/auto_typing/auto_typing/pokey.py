#!/usr/bin/env python3
# Purpose: Autonomous typing

"""
Slop code to run finger linear actuator.
TODO: Add params for number of can sends in a row and delay between them.
"""

import logging
import rclpy
from rclpy.action import ActionServer
from rclpy.node import Node
import jcan
import time

from nova_interfaces.action import EndEffector


class EndEffectorActionServer(Node):
    CAN_BUS = "can1"
    CAN_ID = 0x077
    # cansend can1 077#02
    POKE_FORWARD = 0x02
    # camsemd cam1 077#01
    POKE_BACKWARD = 0x01
    def __init__(self):
        super().__init__('pokey_server')
        self.get_logger().info("Ready to poke!")

        self._action_server = ActionServer(
            self,
            EndEffector,
            '/arm/poke',
            self.execute_callback)

        # for CAN commands
        self.bus = jcan.Bus()
        self.bus.open(self.CAN_BUS)
        #self.timer_spin_can = self.create_timer(0.01, self.bus.spin)

    def execute_callback(self, goal_handle):
        end_poke = goal_handle.request.poke
        print(goal_handle.request)
        forward = False
        if end_poke > 0.5:
            forward = True
        self.get_logger().info(f"Executing end effector goal... poking to {end_poke}")
        feedback_msg = EndEffector.Feedback()


        for i in range(20):
            self.bus.spin()
            self.poke(forward)
            time.sleep(0.25)

        goal_handle.succeed()
        result = EndEffector.Result()
        result.end_poke = end_poke
        return result

    def poke(self, forward:bool):
        if forward:
            # poke forward
            frame = jcan.Frame(self.CAN_ID, [self.POKE_FORWARD])
            self.bus.send(frame)
        else:
            # poke backward
            frame = jcan.Frame(self.CAN_ID, [self.POKE_BACKWARD])
            self.bus.send(frame)
    


def main():
    rclpy.init()
    node = EndEffectorActionServer()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()