from rclpy.node import Node, ParameterDescriptor, Parameter
from rclpy.impl.rcutils_logger import RcutilsLogger
from typing import final, Optional, TypeVar, List, override, overload
from abc import ABC, abstractmethod
from ..common.ControlComponent import ControlComponent
from ..controller_manager.Interface import InterfaceCollection
from ..controller_manager.Contexts import Contexts

T = TypeVar("T")
TController = TypeVar("TController", bound="Controller[object]")

class Controller(ControlComponent[TController]):
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
    # Automatically populated member variables. Populated before even __init__ is called by the DeferredConstructor.
    name: str
    node: Node
    logger: RcutilsLogger

    @abstractmethod
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
            self.__active = False
            return False

        if self.__active:
        return True

    @abstractmethod
    def _on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param command_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        pass

    @final
    def activate(self) -> None:

    def on_activate(self):
        """ Called whenever the Controller becomes active
        :return:
        """
        pass



    @abstractmethod
    def on_update(self, now: float, period: float) -> None:
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass

    def on_deactivate(self):
        """ Called whenever the Controller becomes inactive
        :return:
        """
        pass

    @final
    def set_active(self, active: bool) -> None:
        """ Sets this controller to be active or inactive, updating the associated .active param to reflect the new
        value.
        :param active: Whether the controller should be updated
        """
        if active == self.__active:
            return

        self.__active = active
        # Keep the parameter in sync for any external system that tracks the param
        self.node.set_parameters_atomically([
            Parameter(f"controllers.{self.name}.active", Parameter.Type.BOOL, active)
        ])

        if active:
            self.on_activate()
        else:
            self.on_deactivate()

    @final
    def on_set_parameters_callback(self, params: List[Parameter]) -> None:
        """ Callback method for when parameters change. Calls self.on_set_parameters
        :param params: The list of parameters that have changed --
                       all names are still prefixed with controllers.{self.name}.
        """
        # Check for any changes to being active
        new_active = self.get_parameter("active").value
        if new_active != self.__active:
            self.__active = new_active
            if new_active:
                self.on_activate()
            else:
                self.on_deactivate()

        self._on_set_parameters(params)

    def _on_set_parameters(self, params: List[Parameter]) -> None:
        """ Virtual method called whenever parameters for this controller are updated.
        :param params: The list of parameters that have changed --
                       all names are still prefixed with controllers.{self.name}.
        """
        pass

    @final
    def declare_parameter(self, name: str, initial_value: T, description: str="") -> Parameter:
        """ Declare and initialize a parameter. """
        return self.node.declare_parameter(f"controllers.{self.name}.{name}", initial_value, ParameterDescriptor(description=description))

    @final
    def get_parameter(self, name: str) -> Parameter:
        """ Get a parameter by name. """
        return self.node.get_parameter(f"controllers.{self.name}.{name}")


class TestController(Controller):
    def __init__(self, contexts: Contexts, thing: int):
        pass

    def on_update(self, now: float, period: float) -> None:
        pass

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[
        bool]:
        pass
