#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Control for the URC Pumps
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: urc_pumps
ACTIONS:
    - "/science/pumps_action"   [Pumps]
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
import time
from enum import Enum
from typing import Optional

import rclpy
from rclpy.node import Node
from rclpy.action import ActionServer, GoalResponse, CancelResponse
from rclpy.action.server import ServerGoalHandle

from python_control2 import PythonControl, Controller, Contexts, InterfaceCollection, Interface
from python_control2.hardware_interfaces import QCMDHardware
from science_interfaces.action import Pumps


class PumpState(Enum):
    """State machine states for pump operation"""
    IDLE = 0
    RUNNING = 1


class PumpsController(Controller):
    """
    Controller for URC science pumps using Python Control2 framework.

    Manages two pumps:
    - Cache to Shot Pump: Used for fill_shots action
    - Shot to Carousel Pump: Used for fill_cuvettes_prime and fill_cuvettes actions

    Exposes a ROS2 Action Server at /science/pumps_action that accepts:
    - fill_shots: Runs cache-to-shot pump
    - fill_cuvettes_prime: Runs shot-to-carousel pump
    - fill_cuvettes: Runs shot-to-carousel pump
    """

    # Action names (matching original implementation)
    FILL_SHOTS = "fill_shots"
    FILL_CUVETTES_PRIME = "fill_cuvettes_prime"
    FILL_CUVETTES = "fill_cuvettes"

    # Default durations for each action (seconds)
    DEFAULT_DURATIONS = {
        FILL_SHOTS: 10.0,
        FILL_CUVETTES_PRIME: 10.0,
        FILL_CUVETTES: 10.0,
    }

    # Timeout in iterations (at 10Hz sleep rate = 120 seconds)
    TIMEOUT_ITERATIONS = 1200

    def __init__(self, contexts: Contexts,
                 action_name: str = "/science/pumps_action",
                 cache_pump_hardware: str = "cache_to_shot_pump",
                 carousel_pump_hardware: str = "shot_to_carousel_pump",
                 max_effort: float = 0.75,
                 default_duration: float = 10.0):
        """
        Constructor for PumpsController.

        :param contexts: Dependency injection contexts from Python Control2
        :param action_name: ROS action server name
        :param cache_pump_hardware: Hardware interface name for cache-to-shot pump
        :param carousel_pump_hardware: Hardware interface name for shot-to-carousel pump
        :param max_effort: Maximum pump effort (0.0 to 1.0)
        :param default_duration: Default run duration in seconds
        """
        super().__init__(contexts)

        # Declare parameters (can be overridden via ROS params)
        self.action_name: str = self.declare_parameter("action_name", action_name).value
        self.cache_pump_name: str = self.declare_parameter("cache_pump_hardware", cache_pump_hardware).value
        self.carousel_pump_name: str = self.declare_parameter("carousel_pump_hardware", carousel_pump_hardware).value
        self.max_effort: float = self.declare_parameter("max_effort", max_effort).value
        self.default_duration: float = self.declare_parameter("default_duration", default_duration).value

        # State machine for timed pump operations
        self.state: PumpState = PumpState.IDLE
        self.current_goal_handle: Optional[ServerGoalHandle] = None
        self.current_pump_cmd: Optional[Interface] = None
        self.current_action: Optional[str] = None

        # Timing state
        self.run_start_time: float = 0.0
        self.run_duration: float = 0.0

        # Map action names to pump hardware names
        self.action_to_pump = {
            self.FILL_SHOTS: self.cache_pump_name,
            self.FILL_CUVETTES_PRIME: self.carousel_pump_name,
            self.FILL_CUVETTES: self.carousel_pump_name,
        }

        self.logger.info(f"PumpsController initialized with pumps: {self.cache_pump_name}, {self.carousel_pump_name}")

    def on_configure(self, command_interfaces: InterfaceCollection,
                     state_interfaces: InterfaceCollection) -> Optional[bool]:
        """
        Configure the controller - get interfaces and create action server.

        :param command_interfaces: Collection of command interfaces from hardware
        :param state_interfaces: Collection of state interfaces from hardware
        :returns: True if configured successfully
        """
        # Get command interfaces for both pumps
        self.cache_pump_cmd = command_interfaces[f"{self.cache_pump_name}/effort"]
        self.carousel_pump_cmd = command_interfaces[f"{self.carousel_pump_name}/effort"]

        # Validate interfaces
        if not self.cache_pump_cmd:
            self.logger.warning(f"Cache pump command interface '{self.cache_pump_name}/effort' not populated")
        if not self.carousel_pump_cmd:
            self.logger.warning(f"Carousel pump command interface '{self.carousel_pump_name}/effort' not populated")

        # Create the action server
        self.action_server = ActionServer(
            self.node,
            Pumps,
            self.action_name,
            execute_callback=self.execute_callback,
            goal_callback=self.goal_callback,
            cancel_callback=self.cancel_callback,
        )

        self.logger.info(f"PumpsController configured with action server at '{self.action_name}'")
        return True

    def goal_callback(self, goal_request) -> GoalResponse:
        """
        Handle incoming goal requests.

        :param goal_request: The goal request from the action client
        :returns: GoalResponse indicating accept or reject
        """
        pump_action = goal_request.pump

        # Validate the action name
        if pump_action not in self.action_to_pump:
            self.logger.error(f"Invalid pump action requested: '{pump_action}'")
            return GoalResponse.REJECT

        # Reject if already running
        if self.state != PumpState.IDLE:
            self.logger.warning(f"Rejecting goal - pump already running action '{self.current_action}'")
            return GoalResponse.REJECT

        self.logger.info(f"Accepting pump action goal: '{pump_action}'")
        return GoalResponse.ACCEPT

    def cancel_callback(self, goal_handle: ServerGoalHandle) -> CancelResponse:
        """
        Handle cancellation requests.

        :param goal_handle: The goal handle to cancel
        :returns: CancelResponse indicating accept or reject
        """
        self.logger.info("Received cancel request for pump action")
        return CancelResponse.ACCEPT

    def execute_callback(self, goal_handle: ServerGoalHandle) -> Pumps.Result:
        """
        Execute the pump action.

        This method blocks until the pump operation completes. The on_update()
        method runs in a separate thread (via PythonControl's timer) and handles
        the actual pump control and timing.

        :param goal_handle: The goal handle for this action
        :returns: The action result
        """
        pump_action = goal_handle.request.pump
        time_to_run = goal_handle.request.time_to_run

        # Use default duration if not specified
        if time_to_run <= 0:
            time_to_run = self.DEFAULT_DURATIONS.get(pump_action, self.default_duration)

        self.logger.info(f"Executing pump action '{pump_action}' for {time_to_run} seconds")

        # Set up the pump run state
        pump_name = self.action_to_pump[pump_action]
        if pump_name == self.cache_pump_name:
            self.current_pump_cmd = self.cache_pump_cmd
        else:
            self.current_pump_cmd = self.carousel_pump_cmd

        self.current_goal_handle = goal_handle
        self.current_action = pump_action
        self.run_duration = time_to_run
        self.run_start_time = time.time()
        self.state = PumpState.RUNNING

        # Wait for completion - on_update() handles the actual timing
        # Action server runs execute_callback in a separate thread, so this blocks safely
        iteration = 0
        while self.state == PumpState.RUNNING and iteration < self.TIMEOUT_ITERATIONS:
            # Check for cancellation
            if goal_handle.is_cancel_requested:
                self.logger.info(f"Cancelling pump action '{pump_action}'")
                self.stop_all_pumps()
                self.state = PumpState.IDLE
                goal_handle.canceled()
                result = Pumps.Result()
                result.success = False
                self._cleanup_goal_state()
                return result

            # Calculate elapsed time and publish feedback
            elapsed_time = time.time() - self.run_start_time
            feedback_msg = Pumps.Feedback()
            feedback_msg.time_running = float(elapsed_time)
            feedback_msg.time_to_run = float(self.run_duration)
            goal_handle.publish_feedback(feedback_msg)

            self.logger.debug(f"Pump '{pump_action}': {elapsed_time:.1f}s / {self.run_duration:.1f}s")

            # Check if duration completed
            if elapsed_time >= self.run_duration:
                self.logger.info(f"Pump action '{pump_action}' completed after {elapsed_time:.2f}s")
                self.stop_all_pumps()
                self.state = PumpState.IDLE
                break

            time.sleep(0.1)
            iteration += 1

        # Determine success
        elapsed_time = time.time() - self.run_start_time
        success = elapsed_time >= self.run_duration

        # Return result
        result = Pumps.Result()
        result.success = success

        if success:
            goal_handle.succeed()
            self.logger.info(f"Successfully completed pump action '{pump_action}'")
        else:
            goal_handle.abort()
            self.logger.error(f"Failed to complete pump action '{pump_action}' (timeout)")

        self._cleanup_goal_state()
        return result

    def _cleanup_goal_state(self):
        """Clean up state after goal completion."""
        self.current_goal_handle = None
        self.current_pump_cmd = None
        self.current_action = None

    def on_update(self, now: float, period: float):
        """
        Update loop called every control cycle.

        Sets pump effort based on current state. The timing and feedback
        are handled in execute_callback since ActionServer runs it in
        a separate thread.

        :param now: Current time in seconds
        :param period: Time since last update in seconds
        """
        if self.state == PumpState.IDLE:
            # Ensure pumps are stopped when idle
            self.cache_pump_cmd.value = 0.0
            self.carousel_pump_cmd.value = 0.0
            return

        if self.state == PumpState.RUNNING:
            # Set active pump effort
            if self.current_pump_cmd:
                self.current_pump_cmd.value = self.max_effort

            # Ensure other pump is stopped
            if self.current_pump_cmd == self.cache_pump_cmd:
                self.carousel_pump_cmd.value = 0.0
            else:
                self.cache_pump_cmd.value = 0.0

    def stop_all_pumps(self):
        """Stop all pump motors by setting effort to 0."""
        self.cache_pump_cmd.value = 0.0
        self.carousel_pump_cmd.value = 0.0
        self.logger.debug("All pumps stopped")


def main():
    rclpy.init()

    node = Node("urc_pumps")

    PythonControl(node, update_rate=10, can_bus="can1") \
        .with_controller(
            "pumps_controller",
            PumpsController,
            action_name="/science/pumps_action",
            cache_pump_hardware="cache_to_shot_pump",
            carousel_pump_hardware="shot_to_carousel_pump",
            max_effort=0.75,
            default_duration=10.0
        ) \
        .with_hardware("cache_to_shot_pump", QCMDHardware, can_id=0x031) \
        .with_hardware("shot_to_carousel_pump", QCMDHardware, can_id=0x032) \
        .with_jcan() \
        .spin()


if __name__ == "__main__":
    main()
