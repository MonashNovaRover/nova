import jcan, logging
from rclpy.node import Node, ParameterDescriptor
from typing import Type, TypeVar

from .ControllerManager import ControllerManager

T = TypeVar("T")

class ControllerManagerBuilder:

    def __init__(self, controller_manager: ControllerManager):
        self._cm = controller_manager

    @classmethod
    def NewControllerManager(cls, system_name: str) -> ControllerManagerBuilder:
        cm = ControllerManager(system_name)

        cmb = ControllerManagerBuilder(cm)
        cmb.with_context(Node, name=system_name)

        node = cm.contexts[Node]
        logging_level = node.declare_parameter("logging_level", "INFO", ParameterDescriptor(name="Logging level.")).value
        node.get_logger().set_level(logging.getLevelNamesMapping()[logging_level])

        return cmb

    def with_hardware(self, name: str, hardware_class) -> ControllerManagerBuilder:
        self._cm.hardware_interface.append(hardware_class(name, self._cm.contexts))
        return self

    def with_controller(self, name: str, controller_class) -> ControllerManagerBuilder:
        return self

    def with_context(self, cls: Type[T], *args, **kwargs) -> ControllerManagerBuilder:
        """ Adds a context to the control managers contexts. You can either provide an instance of the class, or
        argumnets to construct the class.

        :param cls: The class you want to declare the value for
        :param args: The instance of the class to use, OR the arguments to construct the class instance with
        :param kwargs: Names args used to construct the class instance with. Leave empty if you want to use an instance
        of the class, rather than constructing one.
        """
        if len(args) == 1 and len(kwargs) == 0 and isinstance(cls, args[0]):
            # Special case, allowing you to pass in an instance of the class directly.
            self._cm.contexts[cls] = args[0]
            return self

        self._cm.contexts.construct(cls, *args, **kwargs)
        return self

    def with_jcan(self) -> ControllerManagerBuilder:
        can_bus = self._cm.contexts[Node].declare_parameter("can_bus", "can1", ParameterDescriptor(description="CAN Bus."))
        # jcan_spin_speed = self.node.declare_parameter("jcan_update_rate", 100, ParameterDescriptor(name="How often to spin jcan per second."))

        self.with_context(jcan.Bus)
        self._cm.contexts[jcan.Bus].open(can_bus)

        return self

    def with_teleop(self) -> ControllerManagerBuilder:
        return self

    def spin(self):
        self._cm.contexts[Node].get_logger().info(f"Starting with State In")
        pass