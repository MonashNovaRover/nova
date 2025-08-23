from rclpy.node import Node, ParameterDescriptor
from typing import TypeVar
from abc import ABC, abstractmethod
from python_control2.controller_manager.Interface import Interface

T = TypeVar("T")

class Controller(ABC):

    def __init__(self, name, contexts):
        self.name = name
        self.node: Node = contexts[Node]
        self.logger = self.node.get_logger()

    @abstractmethod
    def on_configure(self, command_interfaces: Interface, state_interfaces: Interface):
        pass

    @abstractmethod
    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
        here.
        """
        pass

    def declare_parameter(self, name: str, initial_value: T, description: str=""):
        """ Declare and initialize a parameter. """
        return self.node.declare_parameter(f"controllers.{self.name}.{name}", initial_value, ParameterDescriptor(description=description))

    def get_parameter(self, name: str):
        """ Get a parameter by name. """
        return self.node.get_parameter(f"controllers.{self.name}.{name}")
