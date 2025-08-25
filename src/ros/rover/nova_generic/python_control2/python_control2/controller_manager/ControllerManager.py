from typing import Optional

import rclpy
from rcl_interfaces.msg import ParameterDescriptor
from rclpy import Parameter

from ..controller_manager.Contexts import Contexts
from ..controller_manager.Interface import Interface, InterfaceCollection
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
        self.state_interfaces: InterfaceCollection = InterfaceCollection()
        self.command_interfaces: InterfaceCollection = InterfaceCollection()

        # Called before hardware interfaces read from hardware
        self.on_read: Event[[]] = Event()
        # Called before controllers attempt to update
        self.on_update: Event[[]] = Event()
        # Called before hardware interfaces write to hardware
        self.on_write: Event[[]] = Event()

        # The node used by this controller manager. Only guaranteed to have a value after being spun.
        self.node: Optional[Node] = None
        self._update_rate_param = None

        # The last recorded update start time
        self._last_now_nanoseconds: int = 0

    def __update_callback(self):
        now_nanoseconds: int = self.node.get_clock().now().nanoseconds
        now: float = now_nanoseconds * 1e-9

        period_nanoseconds: int = now_nanoseconds - self._last_now_nanoseconds
        period: float = period_nanoseconds * 1e-9

        self.update(now, period)

        self._last_now_nanoseconds = now_nanoseconds

    def update(self, now: float, period: float):
        self.on_read.invoke()
        for hardware_interface in self.hardware_interfaces:
            hardware_interface.on_read(now, period)

        self.on_update.invoke()
        for controller in self.controllers:
            controller.on_update(now, period)

        self.on_write.invoke()
        for hardware_interface in self.hardware_interfaces:
            hardware_interface.on_write(now, period)

    def spin(self, default_update_rate: float=20, auto_run_rclpy: bool=True) -> None:
        """ Repeatedly updates until the program ends.
        :param default_update_rate: The rate at which update will be called, in hz.
        :param auto_run_rclpy: When True (the default), rclpy.spin() and rclpy.shutdown() will be called automatically
        :return: None
        """
        # Make sure node is set
        if Node not in self.contexts:
            # TODO: Create node
            self.node = self.contexts.construct(Node, self.system_name)
        elif self.node is None:
            self.node = self.contexts[Node]

        # Get the update rate
        self._update_rate_param: Parameter = self.node.declare_parameter(
            "update_rate", default_update_rate,
            ParameterDescriptor(description="The rate at which update will be called, in hz."))
        update_period = 1 / self._update_rate_param.value

        # Set initial value of _last_now_nanoseconds so we can calculate period for the first update
        self._last_now_nanoseconds: int = self.node.get_clock().now().nanoseconds
        self.node.create_timer(update_period, self.__update_callback)

        if auto_run_rclpy:
            rclpy.spin(self.node)
            rclpy.shutdown()



