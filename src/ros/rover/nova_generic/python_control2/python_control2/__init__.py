from .controller_manager.Interface import Interface, InterfaceCollection
from .controllers.Controller import Controller
from .hardware_interfaces.HardwareInterface import HardwareInterface
from .hardware_interfaces.Direction import Direction
from .controller_manager.Activation import Activation
from .controller_manager.ControllerManagerBuilder import ControllerManagerBuilder
from .controller_manager.ControllerManager import ControllerManager
from .controller_manager.Contexts import Contexts

from rclpy.node import Node

def PythonControl(node: Node, **kwargs) -> ControllerManagerBuilder:
    """ Creates a ControllerManagerBuilder for a system with the given name.

    :param node: The node for your python control system.
    :param kwargs: Allows you to define default parameters as kwargs (i.e. can_bus="can1").
    :return: A new ControllerManagerBuilder.
    """
    return ControllerManagerBuilder.NewControllerManager(node, default_params=kwargs)
