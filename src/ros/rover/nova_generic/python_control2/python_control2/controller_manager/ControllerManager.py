from typing import Optional, Any, final, TypeVar, List, Dict

import rclpy, logging
from rcl_interfaces.msg import ParameterDescriptor
from rclpy import Parameter

from ..controller_manager.Contexts import Contexts
from ..controller_manager.Interface import Interface, InterfaceCollection
from ..controllers.Controller import Controller
from ..hardware_interfaces.HardwareInterface import HardwareInterface
from rclpy.node import Node
from teleop_python_utils import Event

T = TypeVar("T")

class ControllerManager:

    def __init__(self, node: Node, default_params: Optional[dict[str, Any]]=None):
        """ Constructor.
        :param system_name: The name of the control system (e.g. 'auger', 'science_platform', etc.)
        :param default_params: A map of param names to default param values.
        """
        self.system_name = node.get_name()

        if default_params is None:
            default_params = {}
        self.default_params = default_params

        # Set up internals
        # Used to provide dependencies in an extensible way
        self.contexts: Contexts = Contexts()

        # Interfaces
        self.state_interfaces: InterfaceCollection = InterfaceCollection()
        self.command_interfaces: InterfaceCollection = InterfaceCollection()

        # Control Components
        self.hardware_interfaces: list[HardwareInterface] = []
        self.controllers: list[Controller] = []
        self.controller_name_to_idx: Dict[str, int] = {}

        # Events
        # Called before hardware interfaces read from hardware
        self.on_read: Event[[float, float]] = Event()
        # Called before controllers attempt to update
        self.on_update: Event[[float, float]] = Event()
        # Called before hardware interfaces write to hardware
        self.on_write: Event[[float, float]] = Event()
        # Called when parameters change
        self.on_set_parameters: Event[[List[Parameter]]] = Event()

        # The node used by this controller manager.
        self.node = node
        self.contexts[Node] = node
        logging_level = self.node.declare_parameter("logging_level", "INFO", ParameterDescriptor(name="Logging level.")).value
        self.node.get_logger().set_level(logging.getLevelNamesMapping()[logging_level])

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
        self.on_read.invoke(now, period)
        for hardware_interface in self.hardware_interfaces:
            hardware_interface.on_read(now, period)

        self.on_update.invoke(now, period)
        for controller in self.controllers:
            if controller.active:
                controller.on_update(now, period)

        self.on_write.invoke(now, period)
        for hardware_interface in self.hardware_interfaces:
            hardware_interface.on_write(now, period)

    def spin(self, default_update_rate: float=20, auto_run_rclpy: bool=True) -> None:
        """ Repeatedly updates until the program ends.
        :param default_update_rate: The rate at which update will be called, in hz.
        :param auto_run_rclpy: When True (the default), rclpy.spin() and rclpy.shutdown() will be called automatically
        :return: None
        """
        def set_populated(interface: Interface):
            """ Called whenever a controller gets a command interface or a hardware interface gets a state interface.
            """
            interface.populated = True

        # Set up controllers
        self.command_interfaces.on_get_item.add_callback(set_populated)
        for controller in self.controllers:
            result = controller.configure(self.command_interfaces, self.state_interfaces)
            if not result:
                self.controllers.remove(controller)
        self.command_interfaces.on_get_item.remove_callback(set_populated)

        # Set up hardware interfaces
        self.state_interfaces.on_get_item.add_callback(set_populated)
        for hardware_interface in self.hardware_interfaces:
            result = hardware_interface.configure(self.command_interfaces, self.state_interfaces)
            if not result:
                self.hardware_interfaces.remove(hardware_interface)
        self.state_interfaces.on_get_item.remove_callback(set_populated)

        # Get the update rate
        self._update_rate_param: Parameter = self.declare_parameter(
            "update_rate", default_update_rate,
            "The rate at which update will be called, in hz.")
        update_period = 1 / self._update_rate_param.value

        # Set initial value of _last_now_nanoseconds so we can calculate period for the first update
        self._last_now_nanoseconds: int = self.node.get_clock().now().nanoseconds
        self.node.create_timer(update_period, self.__update_callback)

        if auto_run_rclpy:
            rclpy.spin(self.node)
            rclpy.shutdown()

    @final
    def add_controller(self, controller: Controller) -> None:
        """ Adds a controller to self.controllers, making sure controller_name_to_idx is kept up to date.
        :param controller: The controller to add
        """
        self.controller_name_to_idx[controller.name] = len(self.controllers)
        self.controllers.append(controller)

    @final
    def declare_parameter(self, name: str, initial_value: T, description: str="") -> Parameter:
        """ Declare and initialize a parameter. """
        if name in self.default_params:
            initial_value = self.default_params[name]
        return self.node.declare_parameter(name, initial_value, ParameterDescriptor(description=description))

    @final
    def get_parameter(self, name: str) -> Parameter:
        """ Get a parameter by name. """
        return self.node.get_parameter(name)

    @final
    def __on_set_parameters_callback(self, params: List[Parameter]) -> None:
        """ Called whenever any parameter changes on the node.
        :param params: The list of changed parameters
        """
        updated_controller = [False] * len(self.controllers)

        # Update appropriate Controllers
        for param in params:
            name: str = param.name

            if not name.startswith("controllers."):
                continue

            controller_name =



