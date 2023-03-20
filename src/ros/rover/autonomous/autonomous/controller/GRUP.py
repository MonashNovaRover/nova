#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This script is the high-level planner node for the rover 
which receives the destination and processed map, 
then publishing the drive command for movement.
Receives pose updates and waypoints via subscribers
and publishes drive commands. Converted to Ros2 by
Max Tory from initial code by Aidan Pritchard and 
Liam Whittle. Adapted to include extra URC2022 logic. 

Things to watch out for:

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: Planner
TOPICS:
        - Publishes: /autonomous/drive_inputs [DriveInput]
        - Subscribes: /rover/pose [RoverPose]
        - Subscribes: /autonomous/goals [Waypoints]
SERVICES:
        - PathPlanningService 
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        autonomous
AUTHOR(S):      Max Tory, Liam Whittle
CREATION:       07/12/2021
EDITED:         07/03/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# ros import
import rclpy
from rclpy.node import Node
from rclpy.time import Time
from std_srvs.srv import Trigger
from geometry_msgs.msg import PoseStamped, Transform, TransformStamped
from tf2_ros import Buffer, TransformListener

# custom message imports
from core.msg import DriveInput, AutonomousGoal, PivotWheelData
from nav_msgs.msg import Path
from autonomous.controller.spin_controller import SpinController

# autonomous imports
from autonomous.math_utils.controller_math import *
import autonomous.math_utils.transform as transform
from autonomous.config.runtime_params import *
from autonomous.config.ros_config import *
from autonomous.controller.drive_controller import DriveController, TurningMode

# misc
from enum import Enum
import logging


class PlanningState(Enum):
    GPS_HONING = 0  # honing in on a GPS coordinate
    AR_HONING = 1  # honing in on an AR tag
    SEARCH = 2  # searching for an AR tag
    GATE = 3  # passing through a gate
    SUCCESS = 4  # waiting for instruction


class SavedPlanningState:
    def __init__(self, logger):
        # passed from parent
        self._logger = logger
        self.num_paths_planned = 0
        self.state = PlanningState.SUCCESS

    def update_state(self, _state: PlanningState):
        """
        Updates the current state and saves to disk
        :param _state: the state we want to set SavedControllerState to as either and int or ControllerState
        """
        if type(_state) is int:
            self.state = PlanningState(_state)
        else:
            self.state = _state
        #with open(self.saved_state_fn, "w") as fp:
        #    fp.write(str(int(self.state.value)))
        self.num_paths_planned = 0

    def get_logger(self):
        """
        Keeps code consistent ;)
        """
        return self._logger


class GRUP(Node):
    """
    Controls the planning of goals for GRUC to drive to. Passes the goals off to the
    path planner to plan. Receives updates about the current pose of the rover and the waypoints to
    navigate between via ros topics autonomous/pose and autonomous/goals. 
    """

    def __init__(self):
        super().__init__('GRUP')

        # set debug to not get shown
        self.get_logger().set_level(logging.INFO)

        # ~~~~~~~~~~ State ~~~~~~~~
        self.planning_state = SavedPlanningState(logger=self.get_logger())
        self.state_current_planning_destination : AutonomousGoal = None

        # ------------- ROS Things ----------
        # tf2
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(buffer=self.tf_buffer, node=self, spin_thread=True)

        # Publishers
        # Planned destination -> we wish to go here, which is the next step on our path to the target
        self.pub_desired_destination = self.create_publisher(PoseStamped, planning_destination_topic, 10)

        # Subscribers
        self.sub_autonomous_goal = self.create_subscription(AutonomousGoal, "/goal_manager/goals",
                                                            self.callback_new_autonomous_goal, 10)
        # service for changing the LED
        #self.srv_led_success = self.create_client(Trigger, "/autonomous/success")
        #self.srv_led_start = self.create_client(Trigger, "/autonomous/start")

        # Timers
        self.planning_timer = self.create_timer(0.5, self.plan)  # update planning state and plan paths

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ State Transition Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def reset_goals_and_waypoints(self):
        """
        sets all original goals, best effort goals and stored paths back to default state
        """
        # original goal could be a GPS coordinates (in local frame),
        # AR tags, gate based goals, or search goals
        self.state_current_planning_destination = None

    def planning_mode_state_transition(self):
        """
        Update current planning mode based on the rest of the state
        """
        current_goal = self.state_current_planning_destination
        if current_goal is None or current_goal.type == AutonomousGoal.GOAL_TYPE_SPIN:
            if self.planning_state.state != PlanningState.SUCCESS:
                # No goals to look for, so we're done
                self.on_planning_state_update(PlanningState.SUCCESS)
        elif self.planning_state.state != PlanningState.GPS_HONING and \
            current_goal.type == AutonomousGoal.GOAL_TYPE_HONING:
            # Going to raw search coordinate
            self.on_planning_state_update(PlanningState.GPS_HONING)
        elif self.planning_state.state != PlanningState.AR_HONING and \
              current_goal.type in [AutonomousGoal.GOAL_TYPE_TAG, AutonomousGoal.GOAL_TYPE_BLOCK]:
            # Going to an AR tag or block
            self.on_planning_state_update(PlanningState.AR_HONING)
        elif self.planning_state.state != PlanningState.GATE and \
            current_goal.type == AutonomousGoal.GOAL_TYPE_GATE:
            # Going through a gate (URC)
            self.on_planning_state_update(PlanningState.GATE)

    def on_planning_state_update(self, new_state: PlanningState):
        """
        :param new_state: PlanningState
        Performs a number of internal downstream state updates in response to a planning update
        """
        old_state = self.planning_state.state
        self.get_logger().info("------ State Transition: " + str(old_state) + " -> " + str(new_state))
        self.planning_state.update_state(new_state)
        if new_state == PlanningState.SUCCESS:
            trigger = Trigger.Request()
            #self.srv_led_success.call_async(trigger)

        if old_state == PlanningState.SUCCESS:
            trigger = Trigger.Request()
            #self.srv_led_start.call_async(trigger)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Simple State Update Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def callback_new_autonomous_goal(self, msg):
        """
        Callback for autonomous_goal topic.
        When this callback is called, it means we have received two different pieces of information:
            - a new goal coordinate to drive to
            - zero, one, or two AR tag ids to travel to:
                - zero means we ignore AR tags (but store them for later), one means we search for the AR tag,
                and two means we are looking for a gate
        """
        # we update the state of AR tag ids so that it can compare AR tags to the ones we care about
        self.get_logger().debug(f"Received new goal: {msg}")
        self.state_current_planning_destination = msg

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Planning Loop Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def plan(self):
        """
        Function to be called on the goal publisher timer
        """
        # update planning mode state - this is the only time in the codebase this function is called
        self.planning_mode_state_transition()

        if self.planning_state.state == PlanningState.SUCCESS:
            self.get_logger().debug("In SUCCESS state, no planning required.")
            return
        else:
            self.get_logger().debug("plan() state is {}".format(self.planning_state.state))

        planning_destination = PoseStamped()
        planning_destination.header.stamp = self.get_clock().now().to_msg()
        planning_destination.header.frame_id = "map"
        planning_destination.pose.orientation.w = 1.0
        planning_destination.pose.position.x = self.state_current_planning_destination.position.x
        planning_destination.pose.position.y = self.state_current_planning_destination.position.y

        self.pub_desired_destination.publish(planning_destination)

def main(args=None):
    rclpy.init(args=args)
    controller = GRUP()
    rclpy.spin(controller)
    controller.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
