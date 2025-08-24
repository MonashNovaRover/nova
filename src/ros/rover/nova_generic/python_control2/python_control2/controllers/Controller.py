from rclpy.node import Node, ParameterDescriptor
from rclpy.impl.rcutils_logger import RcutilsLogger
from typing import TypeVar, final, Optional
from abc import ABC, abstractmethod
from ..controller_manager.Interface import InterfaceCollection
from ..controller_manager.Contexts import Contexts

T = TypeVar("T")

class Controller(ABC):
    """ TODO: Description """
    name: str
    node: Node
    logger: RcutilsLogger

    _initialized: bool

    @final
    def __new__(cls, *args, **kwargs):
        """ Overrides construction of Controller instances to defer calling __init__ until contexts are available. """
        # Allocate instance without calling __init__
        instance = object.__new__(cls)
        # Store the args for later
        instance._deferred_args = args
        instance._deferred_kwargs = kwargs
        instance._initialized = False
        return instance

    def _initialize(self, name: str, node: Node, contexts: Contexts):
        """ Runs __init__ manually.

        :param name: The name of the controller
        :param node: The node used by the controller for params
        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        if self._initialized:
            return self

        self.name = name
        self.node: Node = node
        self.logger = self.node.get_logger()

        # Actually call __init__
        self.__class__.__init__(self, *self._deferred_args, **self._deferred_kwargs, contexts=contexts)

        self._deferred_args = None
        self._deferred_kwargs = None
        self._initialized = True
        return self

    def __init__(self, contexts: Contexts):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the last arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        pass

    @final
    def _configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> bool:
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
