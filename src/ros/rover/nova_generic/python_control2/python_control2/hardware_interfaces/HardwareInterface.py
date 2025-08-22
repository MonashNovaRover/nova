from rclpy.node import Node, ParameterDescriptor

from python_control2.controller_manager.Interface import Interface

class HardwareInterface:

    def __init__(self, name, contexts):
        self.name = name
        self.node: Node = contexts[Node]
        self.logger = self.node.get_logger()

    def on_configure(self, command_interfaces: Interface, state_interfaces: Interface):
        pass

    def on_update(self, delta):
        pass

    def declare_parameter(self, name: str, initial_value, description: str=""):
        return self.node.declare_parameter(f"hardware.{self.name}.{name}", initial_value, ParameterDescriptor(name=description)).value

    def get_parameter(self, name: str):
        return self.node.get_parameter(f"hardware.{self.name}.{name}").value
