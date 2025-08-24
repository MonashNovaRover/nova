from rclpy.node import Node, ParameterDescriptor
from typing import TypeVar, final, Optional
from abc import ABC, abstractmethod
from ..controller_manager.Interface import InterfaceCollection
from ..controller_manager.Contexts import Contexts

T = TypeVar("T")

class Controller(ABC):
    """ TODO: Description """
    def __init__(self):
        """ Constructor. Does nothing. Override on_configure instead to do what you would normally do with this. """
        pass

    @final
    def __configure(self, name: str, node: Node, contexts: Contexts,
                    command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> bool:
        """ Internal method. Do not use. Replaces the constructor.

        :param name: The name of the controller
        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param command_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: True if the controller was successfully configured. False otherwise.
        """
        self.name = name
        self.node: Node = node
        self.logger = self.node.get_logger()

        result = self.on_configure(contexts, command_interfaces, state_interfaces)
        successfully_configured = result is None or result

        if not successfully_configured:
            self.logger.error(f"Failed to configure controller \"{self.name}\".")
            return False

        return True


    @abstractmethod
    def on_configure(self, contexts: Contexts,
                     command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, or any other contexts, and get references to any command or state
        interface you need.

        :param contexts: A collection of dependency injection class instances you can index by class type.
        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param command_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        pass

    @abstractmethod
    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass

    @final
    def declare_parameter(self, name: str, initial_value: T, description: str=""):
        """ Declare and initialize a parameter. """
        return self.node.declare_parameter(f"controllers.{self.name}.{name}", initial_value, ParameterDescriptor(description=description))

    @final
    def get_parameter(self, name: str):
        """ Get a parameter by name. """
        return self.node.get_parameter(f"controllers.{self.name}.{name}")
