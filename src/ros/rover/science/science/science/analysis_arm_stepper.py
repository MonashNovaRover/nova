#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Controller for the science analysis arm which
actuates up and down.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
COMMAND INTERFACES:
  - actuation/effort      [value between -1 and 1]
  - actuation/position    [distance (mm) from zero position]
STATE INTERFACES:
  - actuation/position    [distance (mm) from zero position]
  - distance/position     [distance from ground (mm)]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: AnalysisArmController
TOPICS:
    -  /science/analysis_arm/position     [sensor_msgs/msg/Range]
SERVICES:
    - /science/analysis_arm/zero          [science_interfaces/srv/SetPosition]
    - /science/analysis_arm/set_position  [std_srvs/srv/Trigger]
    - /science/analysis_arm/stop          [std_srvs/srv/Trigger]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
EVENTS:
  - actuation/zero
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        science
AUTHOR(S):      Felicity Matthews
CREATION:       01/02/26
EDITED:         27/02/26
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import math
import rclpy
from rclpy.node import Node
from typing import Optional

from python_control2 import PythonControl, Controller, Interface, Contexts, InterfaceCollection, Activation
from python_control2.hardware_interfaces import StepperHardware
from teleop_python_utils import Inputs, EventCollection
from sensor_msgs.msg import Range
from std_srvs.srv import Trigger, Trigger_Request, Trigger_Response
from science_interfaces.srv import SetPosition, SetPosition_Request, SetPosition_Response


class AnalysisArmController(Controller):
    # Command interfaces
    actuation_effort_cmd: Interface
    actuation_position_cmd: Interface
    actuation_position_state: Interface
    distance_state: Interface

    def __init__(self, contexts: Contexts, hardware_name: str="actuation", actuation_axis: str="analysis_arm_actuation", max_position: float=300.0):
        """ Constructor, deferred until the control manager has been spun.
        If you override this method, and want to add your own arguments, just make sure contexts is the FIRST arg

        :param contexts: A collection of dependency injection class instances you can index by class type.
        """
        super().__init__(contexts)
        self.logger.info(f"AnalysisArmController -- I have been __init__ialized")
        self.active = contexts[Activation]

        self.target_position = 0
        self.moving_to_target = False

        # Define parameters
        self.hardware_name = self.declare_parameter("hardware_name", hardware_name).value
        self.max_position = self.declare_parameter("max_position", max_position, "Maximum allowed joint position (in mm)").value
        self.min_position = 0.0 # Analysis arm stepper will always have zero position at the top.

        # Actuation axis
        self.actuation_axis_name = self.declare_parameter("actuation_axis", actuation_axis).value

        # Get inputs
        inputs = contexts[Inputs]
        self.actuation_axis = inputs.get_axis(self.actuation_axis_name)

        # Setup publisher and publish timer
        self.publisher = self.node.create_publisher(Range, "/science/analysis_arm/position", 10)
        interval = self.declare_parameter("publish_rate", 3, "How many times a second to publish data.").value
        self.publish_timer = self.node.create_timer(interval / 10, self.publish_data)

        # Setup service servers
        self.zero_service = self.node.create_service(Trigger, "/science/analysis_arm/zero", self.zero_callback)
        self.stop_service = self.node.create_service(Trigger, "/science/analysis_arm/stop", self.stop_callback)
        self.set_position_service = self.node.create_service(SetPosition, "/science/analysis_arm/set_position", self.set_position_callback)

        # Get stepper zero event
        self.zero_event = None
        if EventCollection in contexts:
            events = contexts[EventCollection]
            self.zero_event = events.get(f"{self.hardware_name}/zero")
        else:
            self.logger.error("Could not find EventCollection in the python control contexts, cannot zero analysis arm position.")

    def on_configure(self, command_interfaces: InterfaceCollection, state_interfaces: InterfaceCollection) -> Optional[bool]:
        """ Used to set up your Controller. Run once before any other class method.
        Use this method to get data from self.node, and get references to any command or state interface you need.

        :param command_interfaces: A collection of Interfaces used to send messages to hardware. Get any command
        interfaces you need from this, then store them in member variables.
        :param state_interfaces: A collection of Interfaces containing the current state of the robot. Get any state
        interfaces you need from this, then store them in member variables.
        :returns: None or True if configured successfully. False otherwise.
        """
        # Save references to interfaces
        # Command interfaces
        self.logger.info(f"Getting command interfaces: {self.hardware_name}/effort and {self.hardware_name}/position")
        self.actuation_effort_cmd = command_interfaces[f"{self.hardware_name}/effort"]
        self.actuation_position_cmd = command_interfaces[f"{self.hardware_name}/position"]

        # State interfaces
        self.logger.info(f"Getting state interfaces: {self.hardware_name}/position and distance/position")
        self.actuation_position_state = state_interfaces[f"{self.hardware_name}/position"]

    def on_update(self, now: float, period: float):
        """ Called on every update. You should read values from state interfaces, and set values on command interfaces
            here.
        :param now: The current time, in seconds
        :param period: The time elapsed since the last update, in seconds.
        """
        # Update actuation if there is an effort value and is active and is not currently moving to a target pos.
        self.logger.info(str(self.actuation_effort_cmd.value))
        if self.active and abs(self.actuation_effort_cmd.value) > 0.1:
            self.logger.info("am active and have input")
            if self.moving_to_target:
                self.logger.warn("Analysis arm is currently moving to target position. Please stop the movement if effort control is desired.")
            else:
                self.actuation_position_cmd.value = None
                self.actuation_effort_cmd.value = self.actuation_axis.value
                return

        # Check if we have reached the target position.
        if self.target_position == self.actuation_position_state.value:
            self.moving_to_target = False

        # use position control if moving to target
        if self.moving_to_target:
            self.actuation_position_cmd.value = self.target_position
            self.actuation_effort_cmd.value = 0
            return

        # otherwise set to current position
        self.actuation_position_cmd.value = self.actuation_position_state.value


    def publish_data(self):
        """ Publishes the current position """
        msg = Range()
        msg.max_range = self.max_position
        msg.min_range = self.min_position
        msg.range = self.actuation_position_state.value
        msg.variance = 1 if self.moving_to_target else 0    # telemetry of whether it is moving or not.
        self.publisher.publish(msg)

    def zero_callback(self, _: Trigger_Request, response: Trigger_Response):
        """ Zero Callback function when zero service is called """
        try:
            self.target_position = 0
            self.zero_event.invoke()
            self.logger.info("Successfully zeroed Analysis Arm.")
            response.success = True

        except Exception as e:
            self.logger.error(f"An error occurred while attempting to zero the analysis arm: {e}")
            response.success = False

        return response

    def stop_callback(self, _: Trigger_Request, response: Trigger_Response):
        """ Stop Callback function when stop service is called

        Sets current position to target position so that the analysis arm stops
        moving.
        """
        self.target_position = self.actuation_position_state.value
        self.moving_to_target = False
        self.logger.info("Stopped Analysis Arm - current position is now target position.")
        response.success = True
        return response

    def set_position_callback(self, request: SetPosition_Request, response: SetPosition_Response):
        """ Sets the target position when service is called """
        # Check it is within range
        # Allow if it is outside, the hardware interface will deal with limits (it does allow them to be turned off if needed)
        if request.position > self.max_position:
            self.logger.warn(f"Requested position [{request.position}] is greater than max allowed position [{self.max_position}]")

        # Update target position
        self.target_position = request.position
        self.moving_to_target = True
        response.success = True
        self.logger.info(f"Updating target position: {self.target_position}")

        return response


if __name__ == "__main__":
    print("Setting up!")

    rclpy.init()

    node = Node("analysis_arm")
    inputs = Inputs(node).with_topics("/science/input")

    # ARCh analysis arm system with stepper
    PythonControl(node, update_rate=2, can_bus="can1") \
        .with_controller(
            "controller",
            AnalysisArmController,
            actuation_axis="analysis_arm_actuation",
            max_position=270.0
        ) \
        .with_hardware("actuation", StepperHardware, can_id=0x0E6,
            max_position=270.0, use_max_position=True,
            position_to_steps=lambda x: math.floor((x / 0.04) + 0.5), # rounding each time reduces floating point errors.
            steps_to_position=lambda x: round(x * 0.04, 2),
        ) \
        .with_teleop(inputs) \
        .with_activation_buttons(
            start_active=True,
            active_button_name="activate_analysis_arm",
            inactive_button_pool_names=["activate_cbeam"]) \
        .with_jcan() \
        .with_event_collection() \
        .spin()
