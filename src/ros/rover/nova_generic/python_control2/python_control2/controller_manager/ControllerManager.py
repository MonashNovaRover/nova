from typing import Optional

from ..controller_manager.Contexts import Contexts
from ..controller_manager.Interface import Interface
from ..controllers.Controller import Controller
from ..hardware_interfaces.HardwareInterface import HardwareInterface
from rclpy.node import Node
from teleop_python_utils.Event import Event

class ControllerManager:

    def __init__(self, system_name: str):
        self.system_name = system_name
        self.contexts: Contexts = Contexts()
        self.controllers: list[Controller] = []
        self.hardware_interfaces: list[HardwareInterface] = []
        self.state_interfaces: list[Interface] = []
        self.command_interfaces: list[Interface] = []

        # Called before hardware interfaces read from hardware
        self.on_read: Event[[]] = Event()
        # Called before controllers attempt to update
        self.on_update: Event[[]] = Event()
        # Called before hardware interfaces write to hardware
        self.on_write: Event[[]] = Event()

        # The node used by this controller manager. Only guaranteed to have a value after being spun.
        self.node: Optional[Node] = None


    def __update_callback(self):
        now = self.node.get_clock().now()


    def update(self, now: float, period: float):
        self.on_read.invoke()
        for hardware_interface in self.hardware_interfaces:
            hardware_interface.read()

        self.on_update.invoke()
        for controller in self.controllers:
            controller.on_update(self.contexts.now, self.contexts.delta)

        self.on_write.invoke()
        for hardware_interface in self.hardware_interfaces:
            hardware_interface.write()

    def spin(self):
        # Make sure node is set
        if Node not in self.contexts:
            # TODO: Create node
            pass

        # TODO: aiuafdiwo


