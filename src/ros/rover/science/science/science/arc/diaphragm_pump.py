#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
System for the science Diaphragm Pump which
creates a movement of air through the condenser.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - flow/effort    [value between 0 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Binuda Kalugalage
CREATION:       01/02/2026
EDITED:         04/02/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import jcan

import rclpy
from rclpy.node import Node
from python_control2 import PythonControl
from python_control2.controller_manager import Interface, InterfaceCollection, Contexts
from python_control2.controllers import EffortController
from python_control2.hardware_interfaces import HardwareInterface

from struct import pack

class DiaphragmPumpHardware(HardwareInterface):
    effort_cmd: Interface
    can_id: int

    def __init__(self, contexts: Contexts,
                 can_id: int=0x0E5,
                 max_effort: float=1.0,
                 min_effort_can: int=0x00,
                 max_effort_can: int=0xFF):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]

        self.last = None

        self.declare_parameter("can_id", can_id, "CAN ID of the diaphragm pump")
        self.declare_parameter("max_effort", max_effort, "Max percentage of output to send")
        self.declare_parameter("min_effort_can", min_effort_can, "Min CAN message value that can be sent")
        self.declare_parameter("max_effort_can", max_effort_can, "Max CAN message value that can be sent")

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        """ Used to set up your HardwareInterface. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        # Update params
        self.can_id: int = self.get_parameter("can_id").value
        self.max_effort: float = self.get_parameter("max_effort").value
        self.min_effort_can = self.get_parameter("min_effort_can").value
        self.max_effort_can = self.get_parameter("max_effort_can").value

        # Get command interfaces
        self.effort_cmd = command_interfaces[self.name + "/effort"]

        # Validate command interface configuration
        if not self.effort_cmd:
            self.logger.warn(f'DiaphragmPumpHardware "{self.name}" has no populated command interface '
                             f'("{self.name}/effort")')

        # Validate effort range
        if self.max_effort_can <= self.min_effort_can:
            self.logger.error(f'DiaphragmPumpHardware {self.name} has invalid CAN effort range ' 
                              f'min_effort_can={self.min_effort_can}, max_effort_can={self.max_effort_can}')
            return False

        return True

    def on_read(self, now: float, period: float):
        """ Called to read values from hardware, and put them into stored state interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        pass

    def on_write(self, now: float, period: float):
        """ Called to write to hardware using values stored in command interfaces.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        if self.effort_cmd:
            self.bus.send(self.construct_frame())

    def construct_frame(self) -> jcan.Frame:
        """ Construct the jcan Frame based on current command interface """
        # Convert effort to CAN data using max CAN effort
        data = int(self.effort_cmd.value * self.max_effort * self.max_effort_can)

        # Clamp to bounds
        if data > self.max_effort_can:
            data = self.max_effort_can
        elif data < self.min_effort_can:
            data = self.min_effort_can
      
        # Return the constructed frame
        return jcan.Frame(self.can_id, [data])

if __name__ == "__main__":
    print("Setting up!") 

    rclpy.init()

    node = Node("diaphragm_pump")

    PythonControl(node, update_rate=5, can_bus="can1") \
        .with_controller(
            "controller",
            EffortController,
            hardware_name="flow",
            service_name="/science/diaphragm_pump_command",
            topic_name="/science/diaphragm_pump_status"
        ) \
        .with_hardware("flow", DiaphragmPumpHardware) \
        .with_jcan() \
        .spin()