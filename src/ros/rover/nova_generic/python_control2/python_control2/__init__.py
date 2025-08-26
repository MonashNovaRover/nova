from .controller_manager.Interface import Interface, InterfaceCollection
from .controllers.Controller import Controller
from .hardware_interfaces.HardwareInterface import HardwareInterface
from .controller_manager.ControllerManagerBuilder import ControllerManagerBuilder
from .controller_manager.ControllerManager import ControllerManager
from .controller_manager.Contexts import Contexts

def PythonControl(system_name: str, **kwargs) -> ControllerManagerBuilder:
    """ Creates a ControllerManagerBuilder for a system with the given name.

    :param system_name: A name for your python control system.
    :param kwargs: Allows you to define default parameters as kwargs (i.e. can_bus="can1").
    :return: A new ControllerManagerBuilder.
    """
    return ControllerManagerBuilder.NewControllerManager(system_name, default_params=kwargs)
