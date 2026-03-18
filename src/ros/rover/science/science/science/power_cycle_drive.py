#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: PowerCycleController
SERVICES:
    - server: 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        
AUTHOR(S):      
CREATION:       12/03/2026
EDITED:         13/03/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import time
import jcan
import rclpy
from rclpy.node import Node
from rclpy.callback_groups import MutuallyExclusiveCallbackGroup
from science_interfaces.srv import PowerCycle
from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection

class PowerCycleController(Controller):
    def __init__(self, contexts: Contexts, service_name:str = ""):
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]
        self.service_name = self.declare_parameter("service_name", service_name).value

    # Ensures callbacks in this group run one at a time, preventing concurrent execution.
        self.cbg = MutuallyExclusiveCallbackGroup()
        self.srv = self.node.create_service(
            PowerCycle,
            self.service_name,
            self.power_cycle_callback,
            callback_group=self.cbg
        )
    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> bool:
        return True

    def on_update(self, now: float, period: float):
        # We don't need a continuous control loop, we just wait for the service callback to fire.
        pass

    def power_cycle_callback(self, request, response):
        self.node.get_logger().info(f'Power cycling science rails. Sleep: {request.sleep_duration}s')

        try:
            # send 001#01 and 001#10
            self.bus.send(jcan.Frame(0x001, [0x01]))
            self.bus.send(jcan.Frame(0x001, [0x10]))

            # sleep from GUI input
            time.sleep(request.sleep_duration)

            # send 002#01 and 002#10
            self.bus.send(jcan.Frame(0x002, [0x01]))
            self.bus.send(jcan.Frame(0x002, [0x10]))

            response.success = True
            response.message = "Power cycle complete."
        except Exception as e:
            response.success = False
            response.message = f"Failed to send CAN frames: {str(e)}"
            self.node.get_logger().error(response.message)

        return response


if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("power_cycle_drive")

    PythonControl(node, update_rate=5, can_bus="can0") \
        .with_controller("controller", PowerCycleController, service_name = "/science/power_cycle_drive") \
        .with_jcan() \
        .spin()