import rclpy
from rclpy.node import Node
from typing import Optional
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface, HardwareInterface
from science_interfaces.srv import Togggle
from python_control2.hardware_interfaces import ToggleHardware
from teleop_python_utils import Inputs, EventCollection


class LEDController(Controller):

    def __init__(self, contexts: Contexts, ):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        
        #setup service 
        self.toggle_service = self.node.create_service
 
        

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        pass

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass


if __name__ == "__main__":
    rclpy.init()

    node = Node("science_leds")
    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller("controller", LEDController) \
        .with_hardware("vis_spec_central_led", ToggleHardware) \
        .with_jcan() \
        .with_event_collection() \
        .spin()
 
        
        