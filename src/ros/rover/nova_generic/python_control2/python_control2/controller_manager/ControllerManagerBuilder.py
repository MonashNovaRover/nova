import jcan
import rclpy
from rclpy.node import Node
from typing import Type, TypeVar, List, Any, Optional
from teleop_python_utils import Inputs

from .Activation import Activation
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
    def NewControllerManager(cls, node: Node, default_params: Optional[dict[str, Any]]=None) -> "ControllerManagerBuilder":
        cm = ControllerManager(node, default_params)

        if not rclpy.ok():
            print("You should run rclpy.init() before creating python control!")
            rclpy.init()

        cmb = ControllerManagerBuilder(cm)

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
        """ Adds a jcan bus to the control managers contexts. The bus is spun every update loop before on_read is
        called for each hardware interface. CAN Bus defaults to can1.
        """
        can_bus = self._cm.declare_parameter("can_bus", "can1", "CAN Bus.")
        # jcan_spin_speed = self.node.declare_parameter("jcan_update_rate", 100, ParameterDescriptor(name="How often to spin jcan per second."))

        self.with_context(jcan.Bus)
        jcan_bus = self._cm.contexts[jcan.Bus]
        jcan_bus.open(can_bus.value)

        # Spin the bus before calling on_read for each hardware_interface
        self._cm.on_read.add_callback(lambda now, period: jcan_bus.spin())

        return self

    def with_teleop(self, inputs: Inputs) -> "ControllerManagerBuilder":
        """ Adds teleop_modular functionality. Inputs are received by an Inputs object where Buttons and Axes can be
        retrieved.

        :param inputs: teleop python util Inputs
        see the teleop docs for configuration options:
        https://github.com/BaileyChessum/teleop_modular/blob/main/teleop_python_utils/teleop_python_utils/modules/Inputs.py
        """
        self._cm.contexts[Inputs] = inputs
        return self

    def with_activation_buttons(self, start_active: bool=False, active_button_name: str="", inactive_button_pool_names: list[str]=[""]):
        """
        Allows a system to be activatable by adding an activation object to the cm context
        that controllers can use to conditionally run code.

        Can only have one Activation in a python control2 system.

        :param start_active: Whether to start active or not, defaults to False.
        :param active_button_name: Name of button that activates.
        :param inactive_button_pool_names: Name of buttons that deactivate.
        """
        # Declare parameters
        start_active: bool = self._cm.declare_parameter(
            "active",
            start_active,
            'On start node status').value
        active_button_name: str = self._cm.declare_parameter(
            "active_button",
            active_button_name,
            'Button name that activates the system').value
        inactive_button_pool_names: list[str] = self._cm.declare_parameter(
            "inactive_button_pool",
            inactive_button_pool_names,
            'list of button name that deactivates the system').get_parameter_value().string_array_value

        # Get button references
        if Inputs not in self._cm.contexts:
            raise AssertionError("`.with_teleop` must be called before `.with_activation_buttons`")

        inputs = self._cm.contexts[Inputs]
        active_button = inputs.get_button(active_button_name)
        inactive_button_pool = [inputs.get_button(name) for name in inactive_button_pool_names]

        # Create Activation object and add to cm context.
        self._cm.contexts[Activation] = Activation(active_button, inactive_button_pool, start_active)


    def spin(self, default_update_rate: float=20, auto_run_rclpy: bool=True) -> None:
        """ Repeatedly updates until the program ends.
        :param default_update_rate: The rate at which update will be called, in hz.
        :param auto_run_rclpy: When True (the default), rclpy.spin() and rclpy.shutdown() will be called automatically
        :return: None
        """
        self._cm.contexts[Node].get_logger().info(f"Starting python control")

        # Do deferred initialization
        for constructor in self.controller_constructors:
            self._cm.controllers.append(constructor.construct(self._cm.contexts, self._cm.node))
        for constructor in self.hardware_constructors:
            self._cm.hardware_interfaces.append(constructor.construct(self._cm.contexts, self._cm.node))

        self._cm.spin(default_update_rate, auto_run_rclpy)


