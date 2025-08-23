from rclpy.node import Node, ParameterDescriptor

from ..controller_manager.Interface import InterfaceCollection

class HardwareInterface:

    def __init__(self, name, contexts):
        self.name = name
        self.node: Node = contexts[Node]
        self.logger = self.node.get_logger()

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        pass

    def read(self, now: float, period: float):
        pass

    def write(self, now: float, period: float):
        pass

    def declare_parameter(self, name: str, initial_value, description: str=""):
        """Declare and initialize a parameter."""
        return self.node.declare_parameter(f"hardware.{self.name}.{name}", initial_value, ParameterDescriptor(name=description))

    def get_parameter(self, name: str):
        """Get a parameter by name."""
        return self.node.get_parameter(f"hardware.{self.name}.{name}")
