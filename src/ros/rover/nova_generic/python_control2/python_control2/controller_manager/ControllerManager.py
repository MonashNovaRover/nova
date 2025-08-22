from python_control2.controller_manager.Contexts import Contexts
from python_control2.controller_manager.Interface import Interface
from python_control2.controllers.Controller import Controller
from python_control2.hardware_interfaces.HardwareInterface import HardwareInterface

class ControllerManager:

    def __init__(self, system_name: str):
        self.system_name = system_name
        self.contexts = Contexts()
        self.controllers: list[Controller] = []
        self.hardware_interfaces: list[HardwareInterface] = []
        self.state_interfaces: list[Interface] = []
        self.command_interfaces: list[Interface] = []
