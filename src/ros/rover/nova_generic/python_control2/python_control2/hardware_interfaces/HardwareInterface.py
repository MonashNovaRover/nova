from typing import final, TypeVar

from rclpy.impl.rcutils_logger import RcutilsLogger
from rclpy.node import Node, ParameterDescriptor
from abc import ABC, abstractmethod
from ..controller_manager.Interface import InterfaceCollection
from ..controller_manager.Contexts import Contexts
from ..controllers.DeferredConstructor import DeferredConstructor

T = TypeVar("T")


class HardwareInterface(ABC):
    """ A class that has the responsibility of converting measurable SI unit commands to abstract hardware commands
    (i.e. CAN), and converting abstract hardware state values to measurable SI unit state interface values.
    """
    name: str
    node: Node
    logger: RcutilsLogger

    _initialized: bool

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
    def configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        """ Internal method. Do not use. Replaces the constructor.

        :param name: The name of the hardware interface
        :param command_interfaces: A collection of Interfaces containing commands to send to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces used to send the current state of the robot to
        controllers. Get any state interfaces you need from this, then store them in member variables.
        :returns: True if the hardware interface was successfully configured. False otherwise.
        """
        self.on_configure(command_interfaces, state_interfaces)

    @abstractmethod
    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        """ Used to set up your HardwareInterface. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        pass

    @abstractmethod
    def read(self, now: float, period: float):
        """ Called to read values from hardware, and put them into stored state interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass

    @abstractmethod
    def write(self, now: float, period: float):
        """ Called to write to hardware using values stored in command interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass

    @final
    def declare_parameter(self, name: str, initial_value, description: str=""):
        """Declare and initialize a parameter."""
        return self.node.declare_parameter(f"hardware.{self.name}.{name}", initial_value, ParameterDescriptor(name=description))

    @final
    def get_parameter(self, name: str):
        """Get a parameter by name."""
        return self.node.get_parameter(f"hardware.{self.name}.{name}")
