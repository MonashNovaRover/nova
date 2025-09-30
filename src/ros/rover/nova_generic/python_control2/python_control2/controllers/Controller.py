from rclpy.node import Node, ParameterDescriptor
from rclpy.impl.rcutils_logger import RcutilsLogger
from typing import final, Optional, TypeVar
from abc import ABC, abstractmethod
from .DeferredConstructor import DeferredConstructor
from ..controller_manager.Interface import InterfaceCollection
from ..controller_manager.Contexts import Contexts

T = TypeVar("T")

class Controller(ABC):
    """
    Controllers are what you usually write to implement some system on the rover.

    They determine how the robot should move, calculating a new target state for the robot to try reach at some N hz.

    They usually try to deal in simple units:
      - [-1, 1] for effort (i.e 100% of motor capacity backwards to 100% forwards)
      - m/s or radians/s for velocity
      - m or radians for position

    Controllers runs their main logic inside update(), which is called every control cycle. In update(), the controller
    reads the latest hardware state from State Interfaces retrieved in configure(), and writes the desired hardware
    state to the Command Interface retrieved in configure().
    """
    name: str
    node: Node
    logger: RcutilsLogger

    @final
    def __new__(cls, *args, **kwargs):
        """ Overrides construction of Controller instances to defer calling __init__ until contexts are available. """
        return DeferredConstructor(cls, *args, **kwargs)

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        pass

    @final
    def configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> bool:
        """ Internal method. Do not use. Replaces the constructor.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param command_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: True if the controller was successfully configured. False otherwise.
        """
        result = self.on_configure(command_interfaces, state_interfaces)
        successfully_configured = result is None or result

        if not successfully_configured:
            self.logger.error(f"Failed to configure controller \"{self.name}\".")
            return False
        return True

    @abstractmethod
    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[
        bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

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
