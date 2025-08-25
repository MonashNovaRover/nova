import jcan, logging
import rclpy
from rclpy.node import Node, ParameterDescriptor
from typing import Type, TypeVar, List

from .ControllerManager import ControllerManager
from ..controllers.Controller import Controller, DeferredControllerConstructor
from ..hardware_interfaces.HardwareInterface import HardwareInterface

T = TypeVar("T")

class ControllerManagerBuilder:

    def __init__(self, controller_manager: ControllerManager):
        self._cm = controller_manager

        self.controller_constructors: List[DeferredControllerConstructor] = []

    @classmethod
    def NewControllerManager(cls, system_name: str) -> "ControllerManagerBuilder":
        cm = ControllerManager(system_name)

        if not rclpy.ok():
            print("You should run rclpy.init() before creating python control!")
            rclpy.init()

        cmb = ControllerManagerBuilder(cm)
        cmb.with_context(Node, system_name)

        node = cm.contexts[Node]
        logging_level = node.declare_parameter("logging_level", "INFO", ParameterDescriptor(name="Logging level.")).value
        node.get_logger().set_level(logging.getLevelNamesMapping()[logging_level])

        return cmb

    def with_hardware(self, name: str, hardware_interface: HardwareInterface) -> "ControllerManagerBuilder":
        if isinstance(hardware_interface, type):
            hardware_interface = hardware_interface()

        hardware_interface.name = name
        self._cm.hardware_interfaces.append(hardware_interface)
        return self

    def with_controller(self, name: str, controller: Controller, *args, **kwargs) -> "ControllerManagerBuilder":
        if isinstance(controller, type):
            controller = controller(*args, **kwargs)

        controller.name = name
        self.controller_constructors.append(controller)
        return self

    def with_context(self, cls: Type[T], *args, **kwargs) -> "ControllerManagerBuilder":
        """ Adds a context to the control managers contexts. You can either provide an instance of the class, or
        argumnets to construct the class.

        :param cls: The class you want to declare the value for
        :param args: The instance of the class to use, OR the arguments to construct the class instance with
        :param kwargs: Names args used to construct the class instance with. Leave empty if you want to use an instance
        of the class, rather than constructing one.
        """
        if len(args) == 1 and len(kwargs) == 0 and isinstance(args[0], cls):
            # Special case, allowing you to pass in an instance of the class directly.
            self._cm.contexts[cls] = args[0]
            return self

        self._cm.contexts.construct(cls, *args, **kwargs)
        return self

    def with_jcan(self) -> "ControllerManagerBuilder":
        can_bus = self._cm.contexts[Node].declare_parameter("can_bus", "can1", ParameterDescriptor(description="CAN Bus."))
        # jcan_spin_speed = self.node.declare_parameter("jcan_update_rate", 100, ParameterDescriptor(name="How often to spin jcan per second."))

        self.with_context(jcan.Bus)
        jcan_bus = self._cm.contexts[jcan.Bus]
        jcan_bus.open(can_bus.value)
        self._cm.on_read.add_callback(jcan_bus.spin)

        return self

    def with_teleop(self) -> "ControllerManagerBuilder":
        return self

    def spin(self, default_update_rate: float=20, auto_run_rclpy: bool=True) -> None:
        """ Repeatedly updates until the program ends.
        :param default_update_rate: The rate at which update will be called, in hz.
        :param auto_run_rclpy: When True (the default), rclpy.spin() and rclpy.shutdown() will be called automatically
        :return: None
        """
        # Make sure node is set
        if Node not in self._cm.contexts:
            # TODO: Create node
            self._cm.node = self._cm.contexts.construct(Node, self._cm.system_name)
        elif self._cm.node is None:
            self._cm.node = self._cm.contexts[Node]

        self._cm.contexts[Node].get_logger().info(f"Starting python control")

        # Do deferred initialization
        for constructor in self.controller_constructors:
            self._cm.controllers.append(constructor.construct(constructor.name, self._cm.node, self._cm.contexts))
        for controller in self._cm.controllers:
            controller.configure(self._cm.command_interfaces, self._cm.state_interfaces)

        for hardware_interface in self._cm.hardware_interfaces:
            hardware_interface.initialize(hardware_interface.name, self._cm.node, self._cm.contexts)

        self._cm.spin(default_update_rate, auto_run_rclpy)

        pass