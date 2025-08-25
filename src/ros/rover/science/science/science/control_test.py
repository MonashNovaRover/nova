#!/usr/bin/env python3

import rclpy
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface
import random

class TestController(Controller):
    cmd: Interface
    state: Interface

    def __init__(self, contexts: Contexts):
        self.logger.info(f"Controller -- I have been __init__ialized")

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        self.cmd = command_interfaces["cmd"]
        self.state = state_interfaces["state"]

    def on_update(self, now: float, period: float):
        self.cmd.value = 2 * self.state.value
        self.logger.info(f"controller update {self.state.value} -> {self.cmd.value}")

class TestHardware(HardwareInterface):
    state: Interface
    cmd: Interface

    def __init__(self, contexts: Contexts):
        self.logger.info(f"HardwareInterface -- I have been __init__ialized")

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        self.state = state_interfaces["state"]
        self.cmd = command_interfaces["cmd"]

    def on_read(self, now: float, period: float):
        self.state.value = random.uniform(0.0, 10.0)
        self.logger.info(f"read {self.state.value}")

    def on_write(self, now: float, period: float):
        self.logger.info(f"write {self.cmd.value}\n")


if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    PythonControl("control_test", update_rate=1)\
        .with_controller("test_controller", TestController)\
        .with_hardware("test_hw", TestHardware)\
        .spin()