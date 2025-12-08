from enum import Enum

from pandas.io.formats.format import return_docstring
from rclpy.node import Node, ParameterDescriptor, Parameter
from rclpy.impl.rcutils_logger import RcutilsLogger
from typing import final, Optional, List, Any, Type, TypeVar, overload, ParamSpec, Generic, Callable
from abc import ABC, abstractmethod

from typing_extensions import override

from .DeferredConstructorBase import DeferredConstructorBase
from ..controller_manager.Interface import InterfaceCollection
from ..controller_manager.Contexts import Contexts

TControlComponent = TypeVar("TControlComponent", bound="ControlComponent[ControlComponent, Any]")
PNew = ParamSpec("PNew")

T = TypeVar("T")

MaybeErr = Optional[str]

class ControlComponent(ABC, Generic[TControlComponent, PNew]):

    class State(Enum):
        UNINITIALIZED = 0
        UNCONFIGURED  = 1
        INACTIVE      = 2
        ACTIVE        = 3
        ERROR         = 4   #< Failed to change state for whatever reason
        FINALIZED     = 5   #< Finished

    # Automatically populated member variables. Populated before even __init__ is called by the DeferredConstructor.
    name: str
    node: Node
    logger: RcutilsLogger
    __state: State
    __starts_active: bool

    class DeferredConstructor(DeferredConstructorBase[TControlComponent, PNew, [Contexts]], Generic[TControlComponent, PNew]):
        def __init__(self, cls: Type[TControlComponent], *deferred_args: PNew.args, **deferred_kwargs: PNew.kwargs):
            super().__init__(cls, *deferred_args, **deferred_kwargs)

        @override
        def init_instance(self, contexts: Contexts) -> None:
            super().init_instance(contexts)

        @override
        def construct_without_init(self, name: str, node: Node) -> None:
            super().construct_without_init()

            self.instance.name = name
            self.instance.node = node
            self.instance.logger = node.get_logger().get_child(name)
            self.instance.__state = ControlComponent.State.UNINITIALIZED
            self.instance.__starts_active = self.instance.declare_parameter("active", True).value

    def __new__(cls, *args, **kwargs) -> DeferredConstructor[TControlComponent, PNew]:
        """ Overrides construction of Controller instances to defer calling __init__ until contexts are available.
        :param args: Any args to your constructor
        :param kwargs: Any args to your constructor
        """
        return ControlComponent.DeferredConstructor(cls, *args, **kwargs)

    @abstractmethod
    def __init__(self, contexts: Contexts, *args: PNew.args, **kwargs: PNew.kwargs):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        pass

    @property
    def state(self) -> State:
        """ Accessor for State """
        return self.__state

    @property
    def active(self) -> bool:
        """ Shorthand for checking if state == ACTIVE """
        return self.__state == ControlComponent.State.ACTIVE

    @property
    def finalized(self) -> bool:
        """ Shorthand for checking if state == FINALIZED """
        return self.__state == ControlComponent.State.FINALIZED

    @property
    def starts_active(self) -> bool:
        """ Accessor for State """
        return self.__starts_active

    @abstractmethod
    def _get_role_name(self) -> str:
        """ Should return 'controller' or 'hardware_interface' """
        pass

    @final
    def configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        """ Internal method. Do not use. Replaces the constructor.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param command_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: True if the controller was successfully configured. False otherwise.
        """
        previously_active = self.active
        if previously_active:
            self.deactivate()

        handler = lambda: self._on_configure(command_interfaces, state_interfaces)


        if self.__try_transition(handler, ControlComponent.State.INACTIVE):
            return True


        if self.__active:
            return True

    @abstractmethod
    def _on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> MaybeErr:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param command_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        pass

    def activate(self) -> bool:
        if self.__state == ControlComponent.State.ACTIVE:
            self.logger.warn(f"Tried to deactivate {self.name} while it was already active.")
            return True
        elif self.__state != ControlComponent.State.INACTIVE:
            self.logger.warn(f"Tried to activate() {self.name} while it is in state {self.__state.name}.\nIt must be in"
                             f" the INACTIVE state, which can be reached by calling configure() while in state "
                             f"UNCONFIGURED or ERROR.")
            return False

        # Make transition
        if self.__try_transition(self._on_activate, ControlComponent.State.ACTIVE):
            return True

    def _on_activate(self) -> None:
        """ Called whenever the Controller becomes active
        """
        pass

    def deactivate(self) -> None:
        if self.__state != ControlComponent.State.ACTIVE:
            self.logger.warn(f"Tried to deactivate {self.name} while it was in state {self.__state.name}.")
            return

        # Make transition
        if not self.__try_transition_repeat(self._on_deactivate, ControlComponent.State.INACTIVE):
            self.logger.error(f"Failed to deactivate {self.name}")

    def _on_deactivate(self):
        """ Called whenever the Controller becomes inactive after being active
        :return:
        """
        pass

    def __try_transition(self, handler: Callable[[], None], target_state: State) -> bool:
        """ Calls a handler function for a state transition, and catches any thrown exceptions, transitioning to an
        error state if any are thrown with appropriate log messages.
        :param handler: The callback function attempting to be called
        :param target_state: The state the handler attempts to transition to
        """
        previous_state = self.__state
        try:
            handler()
            self.__state = target_state
            return True
        except Exception as err:
            recovered = self._handle_error(err, target_state)
            if not recovered:
                self.finalize()
            return False

    def __try_transition_repeat(self, handler: Callable[[], None], target_state: State, repeats: int = 2) -> bool:
        """ Calls self.__try_transition N times, or until the transition succeeds or error recovery fails.
        Between each attempt at calling handler where _on_error succeeds, self.__state will be reset to the original
        state the function was called with. If all attempts at calling handler fail, but all calls to _on_error succeed,
        then the final state of the component will be ERROR. If any calls to _on_error fail, the final state will be
        FINALIZED
        """
        previous_state = self.__state

        for i in range(repeats):
            try:
                handler()
                self.__state = target_state
                return True
            except Exception as err:
                recovered = self._handle_error(err, target_state)
                if recovered:
                    self.__state = previous_state
                else:
                    self.finalize()
                    return False

        self.__state = ControlComponent.State.ERROR
        return False

    @final
    def _handle_error(self, err: Exception, target_state: State) -> bool:
        """ Called when a transition fails, logs the exception that occurred, then calls self._on_error to attempt
        recovery.
        :param err: The exception that occurred while attempting to transition state
        :param target_state: the state that we attempted to reach
        """
        previous_state = self.__state
        self.__state = ControlComponent.State.ERROR
        self.logger.error(f"{self.name} threw unhandled {err.__class__.__name__} while transitioning from "
                          f"{previous_state.name} to {target_state.name}:\n{err}")

        recovered = False

        try:
            recovered = self._on_error(err, previous_state, target_state)
        except Exception as err2:
            self.logger.error(f"{self.name}._on_error threw unhandled {err2.__class__.__name__} while attempting to "
                              f"recover from {err.__class__.__name__}:\n{err}")

        return recovered

    def _on_error(self, err: Exception, previous_state: State, target_state: State) -> bool:
        """ Called whenever an exception occurs during any transition.
        :param err: The exception that occurred while attempting to transition state
        :param previous_state: The previous state before attempting to transition
        :param target_state: The state that we attempted to reach
        :returns: True if the error is recoverable, and we can reconfigure this component. False if this error is
        unrecoverable, and all activity from this component should halt.
        """
        return True

    def finalize(self):
        """ Called when something went wrong, and could not be recovered from. """
        if self.active:
            self.deactivate()

        try:
            self._on_finalize()
            return True
        except Exception as err:
            recovered = self._handle_error(err, target_state)
            if not recovered:
                self.finalize()
            return False

    def _on_finalize(self):
        """ Called when something went wrong, and could not be recovered from. """


    @final
    def on_set_parameters_callback(self, params: List[Parameter]) -> None:
        """ Callback method for when parameters change. Calls self.on_set_parameters
        :param params: The list of parameters that have changed --
                       all names are still prefixed with controllers.{self.name}.
        """
        # Check for any changes to being active
        # new_active = self.get_parameter("active").value
        # if new_active != self.__active:
        #     self.__active = new_active
        #     if new_active:
        #         self.on_activate()
        #     else:
        #         self.on_deactivate()

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
        return self.node.declare_parameter(f"{self._get_role_name()}s.{self.name}.{name}",
                                           initial_value, ParameterDescriptor(description=description))

    @final
    def get_parameter(self, name: str) -> Parameter:
        """ Get a parameter by name. """
        return self.node.get_parameter(f"{self._get_role_name()}s.{self.name}.{name}")
