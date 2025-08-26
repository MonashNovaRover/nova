import jcan, logging
import rclpy
from rclpy.node import Node, ParameterDescriptor
from typing import Type, TypeVar, List, Any, Optional
from teleop_python_utils.Inputs import Inputs
from .ControllerManager import ControllerManager
from ..controllers.Controller import Controller
from ..controllers.DeferredConstructor import DeferredConstructor
from ..hardware_interfaces.HardwareInterface import HardwareInterface

T = TypeVar("T")

class ControllerManagerBuilder:

    def __init__(self, controller_manager: ControllerManager):
        self._cm = controller_manager

        self.controller_constructors: List[DeferredConstructor] = []
        self.hardware_constructors: List[DeferredConstructor] = []

    @classmethod
    def NewControllerManager(cls, system_name: str, default_params: Optional[dict[str, Any]]=None) -> "ControllerManagerBuilder":
        cm = ControllerManager(system_name, default_params)

        if not rclpy.ok():
            print("You should run rclpy.init() before creating python control!")
            rclpy.init()

        cmb = ControllerManagerBuilder(cm)
        cmb.with_context(Node, system_name)

        node = cm.contexts[Node]
        logging_level = node.declare_parameter("logging_level", "INFO", ParameterDescriptor(name="Logging level.")).value
        node.get_logger().set_level(logging.getLevelNamesMapping()[logging_level])

        return cmb

    def with_hardware(self, name: str,
                      hardware_interface: Type[HardwareInterface] | DeferredConstructor[HardwareInterface] | HardwareInterface,
                      *args, **kwargs) -> "ControllerManagerBuilder":
        """ Adds a controller to the controller manager.

        :param name: The name to give to the hardware interface 
        :param hardware_interface: Either:
          - the hardware interface class,
          - a deferred constructor for the hardware interface (i.e. if you call `ExampleHardwareInterface()`),
          - or a hardware interface instance (e.g. if you call `ExampleHardwareInterface().construct(contexts)`)
        :param args: Any additional arguments to construct the hardware interface with (excluding contexts)
        :param kwargs: Any additional keyword arguments to construct the hardware interface with (excluding contexts)
        :returns: self
        """
        if isinstance(hardware_interface, type):
            # Type check
            if not issubclass(hardware_interface, HardwareInterface):
                raise ValueError(f"${hardware_interface.__name__} must be a subclass of HardwareInterface.")

            hardware_interface = hardware_interface(*args, **kwargs)

        if isinstance(hardware_interface, DeferredConstructor):
            constructor = hardware_interface

            # Type check
            if not issubclass(constructor.cls, HardwareInterface):
                raise ValueError(f"${constructor.cls.__name__} must be a subclass of HardwareInterface.")

            constructor.name = name
            self.hardware_constructors.append(constructor)
            return self

        if isinstance(hardware_interface, HardwareInterface):
            self._cm.hardware_interfaces.append(hardware_interface)
            return self

        type_name = hardware_interface.__name__ if isinstance(hardware_interface, type) \
            else f"{hardware_interface.__class__.__name__} instance"
        raise TypeError(f"Unsupported hardware_interface argument type given to .with_hardware ({type_name}).")

    def with_controller(self, name: str,
                        controller: Type[Controller] | DeferredConstructor[Controller] | Controller,
                        *args, **kwargs) -> "ControllerManagerBuilder":
        """ Adds a controller to the controller manager.

        :param name: The name to give to the controller
        :param controller: Either:
          - the controller class,
          - a deferred constructor for the controller (i.e. if you call `ExampleController()`),
          - or a controller instance (e.g. if you call `ExampleController().construct(contexts)`)
        :param args: Any additional arguments to construct the controller with (excluding contexts)
        :param kwargs: Any additional keyword arguments to construct the controller with (excluding contexts)
        :returns: self
        """
        if isinstance(controller, type):
            # Type check
            if not issubclass(controller, Controller):
                raise TypeError(f"${controller.__name__} must be a subclass of Controller.")

            controller = controller(*args, **kwargs)

        if isinstance(controller, DeferredConstructor):
            constructor = controller

            # Type check
            if not issubclass(constructor.cls, Controller):
                raise TypeError(f"${constructor.cls.__name__} must be a subclass of Controller.")

            constructor.name = name
            self.controller_constructors.append(constructor)
            return self

        if isinstance(controller, Controller):
            self._cm.controllers.append(controller)
            return self

        type_name = controller.__name__ if isinstance(controller, type) \
            else f"{controller.__class__.__name__} instance"
        raise TypeError(f"Unsupported controller argument type given to .with_controller ({type_name}).")

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

        # Spin the bus before calling on_read for each hardware_interface
        self._cm.on_read.add_callback(lambda now, period: jcan_bus.spin())

        return self

    def with_teleop(self, inputs: Inputs) -> "ControllerManagerBuilder":
        self._cm.contexts[Inputs] = inputs
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
            self._cm.controllers.append(constructor.construct(self._cm.contexts, self._cm.node))
        for constructor in self.hardware_constructors:
            self._cm.hardware_interfaces.append(constructor.construct(self._cm.contexts, self._cm.node))

        self._cm.spin(default_update_rate, auto_run_rclpy)


