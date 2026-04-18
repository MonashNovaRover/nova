#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control for the URC Pumps
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: pumps
TOPICS:
    - publisher: /science/pumps/status  [PumpStatus]
SERVICES:
    - service: /science/pumps/run       [RunPump]
    - service: /science/pumps/stop      [Trigger]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - cache_to_shot_pump/effort        [value between -1 and 1]
  - shot_to_carousel_pump/effort     [value between -1 and 1]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    science
AUTHOR(S):  Felicity Matthews
CREATION:   13-04-2026
EDITED:     13-04-2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
from typing import Optional, Dict, List

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, QoSHistoryPolicy, QoSDurabilityPolicy
from std_srvs.srv import Trigger

from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface
from python_control2.hardware_interfaces import QCMDHardware
from science_interfaces.msg import PumpStatus
from science_interfaces.srv import RunPump


class PumpsController(Controller):
    """
    Controller for URC science pumps using Python Control2 framework.

    Manages pumps dynamically based on provided hardware list.

    Exposes services for pump control and a topic for status:
    - /science/pumps/run: Start a pump operation
    - /science/pumps/stop: Stop current operation
    - /science/pumps/status: Current pump status (published at 5Hz)
    """

    def __init__(self, contexts: Contexts,
                 run_service_name: str = "/science/pumps/run",
                 stop_service_name: str = "/science/pumps/stop",
                 status_topic_name: str = "/science/pumps/status",
                 pump_hardware_list: List[str] = None,
                 max_effort: float = 0.75,
                 publish_rate: int = 5):
        """
        Constructor for PumpsController.

        :param contexts: Dependency injection contexts from Python Control2
        :param run_service_name: Service name for starting pump operations
        :param stop_service_name: Service name for stopping pump operations
        :param status_topic_name: Topic name for status publishing
        :param pump_hardware_list: List of hardware interface names (e.g., ["cache_to_shot_pump", "shot_to_carousel_pump"])
        :param max_effort: Maximum pump effort (0.0 to 1.0)
        :param publish_rate: Status publish rate in Hz
        """
        super().__init__(contexts)

        # Default hardware list
        if pump_hardware_list is None:
            pump_hardware_list = ["cache_to_shot_pump", "shot_to_carousel_pump"]

        # Declare parameters (can be overridden via ROS params)
        self.run_service_name: str = self.declare_parameter("run_service_name", run_service_name).value
        self.stop_service_name: str = self.declare_parameter("stop_service_name", stop_service_name).value
        self.status_topic_name: str = self.declare_parameter("status_topic_name", status_topic_name).value
        self.pump_hardware_list: List[str] = self.declare_parameter("pump_hardware_list", pump_hardware_list).value
        self.max_effort: float = self.declare_parameter("max_effort", max_effort).value
        self.publish_rate: int = self.declare_parameter("publish_rate", publish_rate).value

        # State for timed pump operations
        self.is_running: bool = False
        self.current_pump: str = ""
        self.run_start_time: float = 0.0
        self.run_duration: float = 0.0

        # Dictionary to store command interfaces (populated in on_configure)
        self.pump_cmds: Dict[str, Interface] = {}

        self.logger.info(f"PumpsController initialized with hardware: {self.pump_hardware_list}")

    def on_configure(self, command_interfaces: InterfaceCollection,
                     state_interfaces: InterfaceCollection) -> Optional[bool]:
        """
        Configure the controller - get interfaces, create services and publisher.

        :param command_interfaces: Collection of command interfaces from hardware
        :param state_interfaces: Collection of state interfaces from hardware
        :returns: True if configured successfully
        """
        # Dynamically get command interfaces for all pumps in the hardware list
        for hardware_name in self.pump_hardware_list:
            interface_name = f"{hardware_name}/effort"
            cmd_interface = command_interfaces[interface_name]

            if not cmd_interface:
                self.logger.warning(f"Pump command interface '{interface_name}' not populated")
            else:
                self.logger.debug(f"Registered pump command interface: {interface_name}")

            self.pump_cmds[hardware_name] = cmd_interface

        # Create services
        self.run_service = self.node.create_service(
            RunPump,
            self.run_service_name,
            self.run_callback
        )
        self.stop_service = self.node.create_service(
            Trigger,
            self.stop_service_name,
            self.stop_callback
        )

        # Create status publisher with persisted QoS
        # TRANSIENT_LOCAL allows late-joining subscribers to receive the last message
        qos_profile = QoSProfile(
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1,
            durability=QoSDurabilityPolicy.TRANSIENT_LOCAL
        )
        self.status_publisher = self.node.create_publisher(PumpStatus, self.status_topic_name, qos_profile)

        self.logger.debug(f"PumpsController configured with services at '{self.run_service_name}', '{self.stop_service_name}'")
        self.logger.debug(f"Status publishing to '{self.status_topic_name}' with persisted QoS")
        return True

    def run_callback(self, request: RunPump.Request, response: RunPump.Response) -> RunPump.Response:
        """
        Handle run pump service requests. Non-blocking - returns immediately.

        :param request: The service request with pump hardware name and duration
        :param response: The service response
        :returns: Response indicating success/failure
        """
        # Check if already running
        if self.is_running:
            response.success = False
            response.message = f"Pump already running: {self.current_pump}"
            self.logger.warning(f"Run request rejected - pump already running: {self.current_pump}")
            return response

        # Validate pump hardware name
        if request.pump not in self.pump_hardware_list:
            response.success = False
            response.message = f"Invalid pump hardware: {request.pump}. Valid hardware: {self.pump_hardware_list}"
            self.logger.error(f"Invalid pump hardware requested: '{request.pump}'")
            return response

        # Validate duration
        if request.duration <= 0:
            response.success = False
            response.message = f"Invalid duration: {request.duration}. Duration must be greater than 0."
            self.logger.error(f"Invalid duration requested: {request.duration} for pump '{request.pump}'")
            return response

        # Start the pump operation
        self.current_pump = request.pump
        self.run_duration = request.duration
        self.run_start_time = self.node.get_clock().now().nanoseconds * 1e-9
        self.is_running = True

        response.success = True
        response.message = f"Started {request.pump} for {request.duration:.2f}s"
        self.logger.info(f"Started pump '{request.pump}' for {request.duration:.2f}s")
        return response

    def stop_callback(self, request: Trigger.Request, response: Trigger.Response) -> Trigger.Response:
        """
        Handle stop pump service requests.

        :param request: The trigger request
        :param response: The trigger response
        :returns: Response indicating success
        """
        if self.is_running:
            self.logger.info(f"Stopping pump '{self.current_pump}'")
        else:
            self.logger.info("Stop requested but no pump was running")

        self.is_running = False
        self.current_pump = ""
        self.stop_all_pumps()
        self.publish_status()

        response.success = True
        response.message = "Pumps stopped"
        return response

    def publish_status(self):
        """Publish current pump status to the status topic."""
        msg = PumpStatus()
        msg.running = self.is_running
        msg.pump = self.current_pump

        if self.is_running:
            current_time = self.node.get_clock().now().nanoseconds * 1e-9
            msg.time_elapsed = float(current_time - self.run_start_time)
            msg.time_target = float(self.run_duration)
        else:
            msg.time_elapsed = 0.0
            msg.time_target = 0.0

        self.status_publisher.publish(msg)

    def on_update(self, now: float, period: float):
        """
        Update loop called every control cycle.

        Handles timing and pump control. When running, checks if duration
        has elapsed and stops the pump automatically.

        Publishes status every update when running, once when finishing, and not while idle.

        :param now: Current time in seconds
        :param period: Time since last update in seconds
        """
        if not self.is_running:
            # Ensure all pumps are stopped when idle
            self.stop_all_pumps()
            return

        # Calculate elapsed time
        elapsed = now - self.run_start_time

        # Check if duration completed
        if elapsed >= self.run_duration:
            self.logger.info(f"Pump '{self.current_pump}' completed after {elapsed:.2f}s")
            self.is_running = False
            self.current_pump = ""
            self.stop_all_pumps()
            self.publish_status()
            return

        # Set active pump effort, ensure all others are stopped
        for hardware_name, cmd_interface in self.pump_cmds.items():
            if hardware_name == self.current_pump:
                cmd_interface.value = self.max_effort
            else:
                cmd_interface.value = 0.0

        # Publish status every update while running
        self.publish_status()

    def stop_all_pumps(self):
        """Stop all pump motors by setting effort to 0."""
        for cmd_interface in self.pump_cmds.values():
            cmd_interface.value = 0.0


def main():
    rclpy.init()

    node = Node("pumps")

    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller(
            "controller",
            PumpsController,
            run_service_name="/science/pumps/run",
            stop_service_name="/science/pumps/stop",
            status_topic_name="/science/pumps/status",
            pump_hardware_list=[
                "cache_to_shot_pump",
                "shot_to_inner_pump",
                "shot_to_outer_pump",
                "shot_to_electrochem_pump",
            ],
            max_effort=0.75,
            publish_rate=5
        ) \
        .with_hardware("cache_to_shot_pump", QCMDHardware, can_id=0x031, send_single_zero=True) \
        .with_hardware("shot_to_inner_pump", QCMDHardware, can_id=0x032, send_single_zero=True) \
        .with_hardware("shot_to_outer_pump", QCMDHardware, can_id=0x041, send_single_zero=True) \
        .with_hardware("shot_to_electrochem_pump", QCMDHardware, can_id=0x042, send_single_zero=True) \
        .with_jcan() \
        .spin()


if __name__ == "__main__":
    main()
