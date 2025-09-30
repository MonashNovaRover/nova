#!/usr/bin/env python3
import jcan
import rclpy
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface
import random

from python_control2.hardware_interfaces import CMDHardware


class TestController(Controller):
    cmd: Interface
    state: Interface
    joint_cmd: Interface

    def __init__(self, contexts: Contexts, joint: str="joint"):
        super().__init__(contexts)
        self.logger.info(f"Controller -- I have been __init__ialized")

        self.joint = self.declare_parameter("joint", joint).value

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        self.cmd = command_interfaces["cmd"]
        self.state = state_interfaces["state"]

        self.logger.info(f"Getting \"{self.joint + "/effort"}\"")
        self.joint_cmd = command_interfaces[self.joint + "/effort"]

    def on_update(self, now: float, period: float):
        self.cmd.value = 2 * self.state.value
        self.joint_cmd.value = self.state.value * 0.1
        self.logger.info(f"{self.state.value} -> {self.cmd.value}")
        self.logger.info(f"{self.state.value} -> {self.joint_cmd.value} ({self.joint})")

class TestHardware(HardwareInterface):
    state: Interface
    cmd: Interface

    def __init__(self, contexts: Contexts):
        super().__init__(contexts)
        self.logger.info(f"HardwareInterface -- I have been __init__ialized")
        self.bus = contexts[jcan.Bus]

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        self.state = state_interfaces["state"]
        self.cmd = command_interfaces["cmd"]

        if not self.cmd:
            self.logger.error("cmd not provided!")
        if self.cmd:
            self.logger.warn("cmd is provided")

    def on_read(self, now: float, period: float):
        self.state.value = random.uniform(0.0, 10.0)
        self.logger.info(f"read {self.state.value}")

    def on_write(self, now: float, period: float):
        self.logger.info(f"write {self.cmd.value}\n")
        frame = jcan.Frame(0x001, [self.cmd.value])
        self.bus.send(frame)


if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    PythonControl("control_test", update_rate=5, can_bus="can1") \
        .with_controller("test_controller", TestController, joint="j1") \
        .with_hardware("test_hw", TestHardware) \
        .with_hardware("j1_cmd", CMDHardware, "j1", can_id=0x1) \
        .with_hardware("j2_cmd", CMDHardware, "j2", can_id=0x1F) \
        .with_hardware("j3_cmd", CMDHardware, "j3", can_id=0x043) \
        .with_jcan() \
        .spin()