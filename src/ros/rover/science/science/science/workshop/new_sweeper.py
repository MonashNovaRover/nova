#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Continuous control for science's sweeper.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: new_sweeper
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SERVICE:    /science/sweeper_command  [EffortCommand]
PUBLISHER:  /science/sweeper_status   [EffortStatus]
COMMAND INTERFACES:
  - sweep/effort    [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      <your name here>
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import math

import jcan
import rclpy
from rclpy.node import Node

from python_control2 import PythonControl, Controller, HardwareInterface, Contexts, Interface, InterfaceCollection
from science_interfaces.msg import EffortStatus
from science_interfaces.srv import EffortCommand


class SweeperHardware(HardwareInterface):
    """ Turns the effort in "sweep/effort" into one signed byte on 0x0E3. """
    effort_cmd: Interface

    def __init__(self, contexts: Contexts, can_id: int = 0x0E3, max_effort_can: int = 0x7F):
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]

        self.declare_parameter("can_id", can_id)
        self.declare_parameter("max_effort_can", max_effort_can)

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        self.can_id: int = self.get_parameter("can_id").value
        self.max_effort_can: int = self.get_parameter("max_effort_can").value

        self.effort_cmd = None  # TODO 3.1.1

        if not self.effort_cmd:
            self.logger.warn(f"Nothing is writing to {self.name}/effort")

    def on_read(self, now: float, period: float):
        pass  # the servo never tells us anything back

    def on_write(self, now: float, period: float):
        pass  # TODO 3.1.3

    def construct_frame(self) -> jcan.Frame:
        # TODO 3.1.2
        return jcan.Frame(self.can_id, [0x00])


class OscillateController(Controller):
    """ Throwaway controller that sweeps effort back and forth, to test the hardware interface. """
    effort_cmd: Interface

    def __init__(self, contexts: Contexts, hardware_name: str = "sweep", period: float = 4.0):
        super().__init__(contexts)
        self.hardware_name = self.declare_parameter("hardware_name", hardware_name).value
        self.period = self.declare_parameter("period", period).value

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        self.effort_cmd = command_interfaces[f"{self.hardware_name}/effort"]

    def on_update(self, now: float, period: float):
        self.effort_cmd.value = math.sin(2 * math.pi * now / self.period)


class SweeperController(Controller):
    """ Owns whether the sweeper is on and how hard it spins. """
    effort_cmd: Interface

    def __init__(self, contexts: Contexts,
                 hardware_name: str = "sweep",
                 service_name: str = "/science/sweeper_command",
                 topic_name: str = "/science/sweeper_status",
                 publish_rate: int = 5):
        super().__init__(contexts)

        self.hardware_name = self.declare_parameter("hardware_name", hardware_name).value
        self.service_name: str = self.declare_parameter("service_name", service_name).value
        self.topic_name: str = self.declare_parameter("topic_name", topic_name).value
        self.publish_rate: int = self.declare_parameter("publish_rate", publish_rate).value

        self.is_on = False
        self.effort_level = 0.0

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        self.effort_cmd = None  # TODO 3.2.1

        # TODO 3.2.2

        # TODO 3.2.3

    def publish_status(self):
        pass  # TODO 3.2.2

    def command_callback(self, request: EffortCommand.Request, response: EffortCommand.Response):
        self.logger.info(f"asked for state={request.state}, level={request.level:.2f}")

        # TODO 3.2.4

        response.success = True
        return response

    def on_update(self, now: float, period: float):
        pass  # TODO 3.2.5


if __name__ == "__main__":
    rclpy.init()

    node = Node("new_sweeper")

    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller("controller", OscillateController, hardware_name="sweep") \
        .with_hardware("sweep", SweeperHardware, can_id=0x0E3) \
        .with_jcan() \
        .spin()

    # TODO 3.2.6
