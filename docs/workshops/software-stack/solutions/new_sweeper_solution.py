#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
SOLUTION for science/workshop/new_sweeper.py.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import jcan
import rclpy
from rclpy.node import Node

from python_control2 import PythonControl, Controller, HardwareInterface, Contexts, Interface, InterfaceCollection
from science_interfaces.msg import EffortStatus
from science_interfaces.srv import EffortCommand


class SweeperHardware(HardwareInterface):
    """ Turns the effort in "sweep/effort" into one signed CAN byte. """
    effort_cmd: Interface

    def __init__(self, contexts: Contexts, can_id: int = 0x0E3, max_effort_can: int = 0x7F):
        super().__init__(contexts)

        self.bus = contexts[jcan.Bus]

        self.declare_parameter("can_id", can_id)
        self.declare_parameter("max_effort_can", max_effort_can)

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        self.can_id: int = self.get_parameter("can_id").value
        self.max_effort_can: int = self.get_parameter("max_effort_can").value

        # STEP 3.1.1
        self.effort_cmd = command_interfaces[f"{self.name}/effort"]

        if not self.effort_cmd:
            self.logger.warn(f"Nothing is writing to {self.name}/effort")

    def on_read(self, now: float, period: float):
        pass

    def on_write(self, now: float, period: float):
        # STEP 3.1.3
        frame = self.construct_frame()
        self.bus.send(frame)

    def construct_frame(self) -> jcan.Frame:
        # STEP 3.1.2
        data = int(self.effort_cmd.value * self.max_effort_can)
        data = max(-self.max_effort_can, min(self.max_effort_can, data))

        return jcan.Frame(self.can_id, list(data.to_bytes(1, byteorder="big", signed=True)))


class SweeperController(Controller):
    """ Owns whether the sweeper is on and how hard it spins. """
    effort_cmd: Interface

    def __init__(self, contexts: Contexts,
                 hardware_name: str = "sweep",
                 service_name: str = "/science/sweeper_command",
                 topic_name: str = "/science/sweeper_status",
                 publish_rate: int = 5):
        super().__init__(contexts)

        self.hardware_name = self.declare_parameter("hardware_name", hardware_name).value
        self.service_name: str = self.declare_parameter("service_name", service_name).value
        self.topic_name: str = self.declare_parameter("topic_name", topic_name).value
        self.publish_rate: int = self.declare_parameter("publish_rate", publish_rate).value

        self.is_on = False
        self.effort_level = 0.0

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection):
        # STEP 3.2.1
        self.effort_cmd = command_interfaces[f"{self.hardware_name}/effort"]

        # STEP 3.2.2
        self.publisher = self.node.create_publisher(EffortStatus, self.topic_name, 10)
        self.publisher_timer = self.node.create_timer(1 / self.publish_rate, self.publish_status)

        # STEP 3.2.3
        self.command_service = self.node.create_service(EffortCommand, self.service_name, self.command_callback)

    def publish_status(self):
        # STEP 3.2.2
        msg = EffortStatus()
        msg.state = self.is_on
        self.publisher.publish(msg)

    def command_callback(self, request: EffortCommand.Request, response: EffortCommand.Response):
        self.logger.info(f"asked for state={request.state}, level={request.level:.2f}")

        # STEP 3.2.4
        self.is_on = request.state
        self.effort_level = request.level

        response.success = True
        return response

    def on_update(self, now: float, period: float):
        # STEP 3.2.5
        self.effort_cmd.value = self.effort_level if self.is_on else 0.0


if __name__ == "__main__":
    rclpy.init()

    node = Node("new_sweeper")

    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller("controller", SweeperController, hardware_name="sweep") \
        .with_hardware("sweep", SweeperHardware, can_id=0x0E3) \
        .with_jcan() \
        .spin()
