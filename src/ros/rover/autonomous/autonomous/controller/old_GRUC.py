#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This script is the controller node for the rover 
which receives the destination and processed map, 
then publishing the drive command for movement.
Receives pose updates and waypoints via subscribers
and publishes drive commands. Converted to Ros2 by
Max Tory from initial code by Aidan Pritchard and 
Liam Whittle. Adapted to include extra URC2022 logic. 

Things to watch out for:
-- asynchronous callbacks
    -- state update only happens on planning callback? 
    -- 

Do we put state transition in before or after planning (on planning cycle frequency)
- publishing plan goals every second
- receiving way point lists every second

-- controller timer only handles actual driving, based on set of current states and variables

If update state just before path plan, not wasting a path planning cycle, and never update while path planning.

Could have a counter for number of paths planned in current state cycle (reset to zero on every state transition)


~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: Controller
TOPICS:
        - Publishes: /autonomous/drive_inputs [DriveInput]
        - Subscribes: /rover/pose [RoverPose]
        - Subscribes: /autonomous/goals [Waypoints]
SERVICES:
        - PathPlanningService 
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        autonomous
AUTHOR(S):      Autonomous subteam
CREATION:       07/12/2021
EDITED:         20/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# ros import
import rclpy
from rclpy.node import Node
from rclpy.time import Time
from rclpy.duration import Duration
from rclpy.logging import LoggingSeverity
from geometry_msgs.msg import Transform, PoseStamped
from std_msgs.msg import Empty
from std_srvs.srv import Trigger
from tf2_ros import Buffer, TransformListener

# custom message imports
from core.msg import DriveInput, AlvarMarker, AutonomousGoal, Point2D, AutonomousGoalArray

# autonomous imports
from autonomous.math_utils.controller_math import *
from autonomous.config.runtime_params import *
from autonomous.config.ros_config import *
from autonomous.controller.ar_tag_manager import ArTagManager

# misc
from enum import Enum


class PlanningState(Enum):
    IDLE = 0  # awaiting a goal
    TO_COORDINATE = 1  # honing in on a GPS coordinate
    SEARCH_SPIN = 2  # searching for an AR tag
    SEARCH = 3  # searching for an AR tag
    TO_AR_TAG = 4  # honing in on an AR tag
    THROUGH_GATE = 5  # passing through a gate
    SUCCESS = 6  # waiting for instruction
    RETURN = 7  # returning autonomously to the last goal
    FAILED = 8  # failed to reach goal - currently unusued


class Controller(Node):
    """
    Controls the movement of the rover between waypoints determined by the path planner.
    Receives updates about the current pose of the rover and the waypoints to
    navigate between via ros topics autonomous/pose and autonomous/goals. Publishes drive
    commands to auto_drive_commands
    """

    def __init__(self):
        super().__init__('autonomous_controller_node')

        # set debug to not get shown
        self.get_logger().set_level(LoggingSeverity.INFO)

        # Params
        self.param_plan_frequency = self.declare_parameter("plan_frequency", 1.0).value
        self.param_coordinate_tolerance = self.declare_parameter("coordinate_tolerance_m", 0.5).value
        self.param_ar_tag_tolerance = self.declare_parameter("ar_tag_tolerance_m", 1.0).value
        self.param_gate_goal_tolerance = self.declare_parameter("gate_tolerance_m", 0.3).value
        self.param_return_goal_tolerance = self.declare_parameter("return_tolerance_m", 5.0).value

        # ~~~~~~~~~~ State ~~~~~~~~
        self.state: PlanningState = PlanningState.IDLE
        self.state_current_goal : AutonomousGoal = None
        self.state_long_term_goal : AutonomousGoal = None
        # Initial return goal at 0, 0
        self.state_return_goal : AutonomousGoal = AutonomousGoal()
        self.state_return_goal.type == AutonomousGoal.GOAL_TYPE_COORDINATE
        self.state_ar_tag_manager = ArTagManager()
        self.state_achieved_goal = False
        self.state_return = False

        # Arrays for storing intermediate goals
        self.state_visited_intermediate_goals = []
        self.state_unvisited_intermediate_goals = []
        self.state_search_goals = []
        self.state_gate_goals = []

        # ------------- ROS Things ----------
        # tf2 buffer and listener
        self.tf_buffer = Buffer(node=self)
        self.tf_listener = TransformListener(self.tf_buffer, self)

        # Publishers
        # Planned destination -> we wish to go here, which is the next step on our path to the target
        self.pub_desired_destination = self.create_publisher(Point2D, planning_destination_topic, 10)

        # Subscribers
        self.sub_controller_goal_override = self.create_subscription(Empty, "/GRUC/goal_achieved", self.callback_controller_goal_override, 10)
        self.sub_ar_tags = self.create_subscription(AlvarMarker, ar_track_topic, self.callback_ar_tag, 10)
        self.sub_autonomous_goal = self.create_subscription(AutonomousGoalArray, auto_goal_topic,
                                                            self.callback_new_autonomous_goal, 10)

        # service for changing the LED
        self.srv_led_success = self.create_client(Trigger, "/autonomous/success")
        self.srv_led_start = self.create_client(Trigger, "/autonomous/start")

        self.srv_return = self.create_service(Trigger, "/autonomous/return", self.callback_return)

        # Timers
        self.planning_timer = self.create_timer(1 / self.param_plan_frequency, self.send_next_goal)  # update planning state and plan paths

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ State Transition Helper Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def has_goal(self) -> bool:
        # Do we have a goal?
        return self.state_current_goal is not None

    def intermediate_goal(self) -> bool:
        return self.state_current_goal.type == AutonomousGoal.GOAL_TYPE_INTERMEDIATE
    
    def at_current_goal(self) -> bool:
        """
        Function determines (if there is a goal) if we are near the goal. If there is no
        planned goal or state_current_goal, we return False, since we 
        must have a goal to be near it. 
        :return: Boolean value of True if we are near goal (and goal exists), False otherwise
        """
        # If we don't have a goal, we can't be at it
        if not self.has_goal():
            return False
        
        # controller told us to skip this goal because it couldn't get to it
        if self.state_achieved_goal:
            return True

        # If we haven't received an override from the controller, we haven't completed a spin
        if self.state == PlanningState.SEARCH_SPIN:
            return False

        # get our pose
        try:
            current_pose : Transform = self.tf_buffer.lookup_transform("map", "base_link", Time(), Duration(nanoseconds=1e8)).transform
        except Exception as e:
            self.get_logger().warn(f"Error in at_current_goal: {e}")
            return False

        dist_to_goal = distance([current_pose.translation.x, current_pose.translation.y], 
                                [self.state_current_goal.position.x, self.state_current_goal.position.y])
                                
        # We are returning to the previous goal
        if self.state == PlanningState.RETURN and self.state_current_goal.type != AutonomousGoal.GOAL_TYPE_INTERMEDIATE:
            return dist_to_goal < self.param_return_goal_tolerance
        # We are going through a gate
        elif self.state == PlanningState.THROUGH_GATE:
            return dist_to_goal < self.param_gate_goal_tolerance
        # We are going to an AR tag
        elif self.state == PlanningState.TO_AR_TAG:
            return dist_to_goal < self.param_ar_tag_tolerance
        # We are going to an intermediate waypoint or coordinate
        else:
            return dist_to_goal < self.param_coordinate_tolerance

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ State Transition Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def planning_mode_state_transition(self):
        """
        Update current planning mode based on the rest of the state. For a visualization of the planning 
        state transition logic, see the state diagram on Lucid here: 
        https://lucid.app/lucidspark/67741129-7fc4-42a9-8c29-8b47f2b29881/edit?viewport_loc=-916%2C-393%2C3240%2C1779%2C0_0&invitationId=inv_222475ed-1c41-47ec-a82e-0f737d617e60
        """

        # Transition from IDLE to TO_COORDINATE when provided with a new goal
        if self.state == PlanningState.IDLE:    
            if self.has_goal():
                self.on_state_update(PlanningState.TO_COORDINATE)

        # Transition from SUCCESS to TO_COORDINATE when provided with a new goal
        if self.state == PlanningState.SUCCESS:
            if self.has_goal():
                self.on_state_update(PlanningState.TO_COORDINATE)

        # If "state_return" is raised as True, transition to RETURN
        if self.state_return:
            self.on_state_update(PlanningState.RETURN)

        # In RETURN state, rover follows intermediate goals back to previous goal. If an intermediate goal 
        # is reached then re-initialize RETURN. If previous goal is reached then transition to IDLE  
        if self.state == PlanningState.RETURN:
            if self.at_current_goal() and self.intermediate_goal():
                self.on_state_update(PlanningState.RETURN)
            if self.at_current_goal() and not self.intermediate_goal():
                self.on_state_update(PlanningState.IDLE)

        # In TO_COORDINATE state, rover follows intermediate goals towards new goal. If an intermediate goal 
        # is reached then re-initialize TO_COORDINATE. If the individual tag or gate tags are located, then
        # transition to TO_AR_TAG or THROUGH_GATE. If new goal is reached without locating tags, transition to SEARCH
        elif self.state == PlanningState.TO_COORDINATE:
            if self.at_current_goal() and self.intermediate_goal():
                self.on_state_update(PlanningState.TO_COORDINATE)
            elif self.at_current_goal() and not self.intermediate_goal() and self.state_ar_tag_manager.found_current_tag():
                self.on_state_update(PlanningState.TO_AR_TAG)
            elif self.at_current_goal() and not self.intermediate_goal() and self.state_ar_tag_manager.found_current_gate():
                self.on_state_update(PlanningState.THROUGH_GATE)
            elif self.at_current_goal() and not self.intermediate_goal() and not \
                    (self.state_ar_tag_manager.found_current_tag() or self.state_ar_tag_manager.found_current_gate()):
                self.on_state_update(PlanningState.SEARCH_SPIN)

        # Transition from SEARCH_SPIN to TO_AR_TAG or THROUGH_GATE if individual tag or gate tags are located.
        # Transition to SEARCH if we have completed the spin
        elif self.state == PlanningState.SEARCH_SPIN:
            if self.state_ar_tag_manager.found_current_tag():
                self.on_state_update(PlanningState.TO_AR_TAG)
            elif self.state_ar_tag_manager.found_current_gate():
                self.on_state_update(PlanningState.THROUGH_GATE)
            elif self.at_current_goal():
                self.on_state_update(PlanningState.SEARCH)

        # Transition from SEARCH to TO_AR_TAG or THROUGH_GATE if individual tag or gate tags are located.
        # Transition to SEARCH_SPIN if we have reached the next intermediate goal
        elif self.state == PlanningState.SEARCH:
            if self.state_ar_tag_manager.found_current_tag():
                self.on_state_update(PlanningState.TO_AR_TAG)
            elif self.state_ar_tag_manager.found_current_gate():
                self.on_state_update(PlanningState.THROUGH_GATE)
            elif self.at_current_goal():
                self.on_state_update(PlanningState.SEARCH_SPIN)

        # Transition from TO_AR_TAG to SUCCESS if reached the tag goal
        elif self.state == PlanningState.TO_AR_TAG:
            if self.at_current_goal():
                self.on_state_update(PlanningState.SUCCESS)

        # Transition from THROUGH_GATE to SUCCESS if reached the gate goal
        elif self.state == PlanningState.THROUGH_GATE:
            if self.at_current_goal() and self.intermediate_goal():
                self.on_state_update(PlanningState.THROUGH_GATE)
            elif self.at_current_goal():
                self.on_state_update(PlanningState.SUCCESS)
     
        # PLACEHOLDER --> add state FAILED if necessary 

    def on_state_update(self, new_state: PlanningState):
        """
        :param new_state: PlanningState
        Performs a number of internal downstream state updates in response to a planning update
        """
        old_state = self.state
        self.get_logger().info(f"------ State Transition: {old_state} -> {new_state}")
        self.get_logger().debug(f"After transition:\n"
                                f"Current Goal: {self.state_current_goal}\n"
                                f"Return Goal: {self.state_return_goal}\n"
                                f"Visited Intermediate Goals: {self.state_visited_intermediate_goals}\n"
                                f"Unvisited Intermediate Goals: {self.state_unvisited_intermediate_goals}\n"
                                f"Gate Goals: {self.state_gate_goals}\n"
                                f"Search Goals: {self.state_search_goals}\n"
                                )
        self.state = new_state

        # Reset trigger state variables
        self.state_return = False
        self.state_achieved_goal = False

        # Handle transition triggered state updates
        # Entering success state means we have reached our goal, set the LED color and current goal to None
        if new_state == PlanningState.SUCCESS:
            trigger = Trigger.Request()
            self.srv_led_success.call_async(trigger)
            self.state_return_goal = self.state_current_goal
            self.state_current_goal = None

        # We aren't doing anything - set current goal to None
        elif new_state == PlanningState.IDLE:
            self.state_current_goal = None

        # We are driving to a coordinate - set the LED red if it isn't already, and if this is an intermediate goal, get the next goal
        elif new_state == PlanningState.TO_COORDINATE:
            if old_state == PlanningState.TO_COORDINATE:
                # We reached an intermediate goal, add it to the list of visited goals
                self.state_visited_intermediate_goals.append(self.state_current_goal)
                # If we have more intermediate goals, set the next one as the current goal
                if len(self.state_unvisited_intermediate_goals > 0):
                    self.state_current_goal = self.state_unvisited_intermediate_goals.pop(0)
                # Otherwise, we will now go to the final goal
                else:
                    self.state_current_goal = self.state_long_term_goal
            else:
                # We are starting a new goal, set the LED to flash red
                trigger = Trigger.Request()
                self.srv_led_start.call_async(trigger)
        
        # We are returning to the previous goal by traversing the intermediate waypoints in reverse
        elif new_state == PlanningState.RETURN:
            if len(self.state_visited_intermediate_goals) > 0:
                # visit intermediate goals in reverse order
                self.state_current_goal = self.state_visited_intermediate_goals.pop(-1)
            else:
                self.state_current_goal = self.state_return_goal
                
        # We are going straight to a detected AR tag 
        elif new_state == PlanningState.TO_AR_TAG:
            self.state_current_goal = self.get_ar_tag_goal()

        elif new_state == PlanningState.THROUGH_GATE:
            if old_state != PlanningState.THROUGH_GATE:
                self.set_gate_goals()
            self.state_current_goal = self.state_gate_goals.pop(0)
            
        elif new_state == PlanningState.SEARCH_SPIN:
            if not old_state == PlanningState.SEARCH:
                self.setup_search()
        
        elif new_state == PlanningState.SEARCH:
            self.state_current_goal = self.state_search_goals.pop(0)

        self.get_logger().debug(f"After transition:\n"
                                f"Current Goal: {self.state_current_goal}\n"
                                f"Return Goal: {self.state_return_goal}\n"
                                f"Visited Intermediate Goals: {self.state_visited_intermediate_goals}\n"
                                f"Unvisited Intermediate Goals: {self.state_unvisited_intermediate_goals}\n"
                                f"Gate Goals: {self.state_gate_goals}\n"
                                f"Search Goals: {self.state_search_goals}\n"
                                )

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Simple State Update Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def callback_new_autonomous_goal(self, msg : AutonomousGoalArray):
        """
        Callback for autonomous_goal topic.
        """
        # we update the state of AR tag ids so that it can compare AR tags to the ones we care about
        self.state_ar_tag_manager.set_goal(msg.goals[-1])
        self.state_long_term_goal = msg.goals[-1]
        self.state_unvisited_intermediate_goals = msg.goals[:-1]
        if len(self.state_unvisited_intermediate_goals) == 0:
            self.state_current_goal = self.state_long_term_goal
        else:
            self.state_current_goal = self.state_unvisited_intermediate_goals.pop(0)

    def callback_controller_goal_override(self, msg):
        self.state_achieved_goal = True

    def callback_return(self, msg):
        self.state_return = True


    def setup_search(self):
        self.search_array_index = 0
        self.search_plan = interpolate_circle_points(self.state_current_planning_destination)

    def set_gate_goals(self):
        assert len(self.ar_tag_manager.ar_tag_goals) == 2
        self.gate_array_index = 0
        gate_mid = self.ar_tag_manager.get_average_goal_pose()

        # negative reciprocal gives perpendicular vector to the vector between the gate
        gate_perpendicular = self.ar_tag_manager.get_gate_normal()
        # Current yaw as vector
        vec_to_gate = gate_mid - np.array([self.state_rover_pose.x, self.state_rover_pose.y])
        # -1 if we are facing away from perpendicular vector, +1 if we are towards it
        direction = -np.sign(np.dot(vec_to_gate, gate_perpendicular))

        # Vector from the middle of the two gate poles to our target
        centre_of_gate_to_target = direction * dist_through_gate_m * gate_perpendicular
        goal_0 = gate_mid + centre_of_gate_to_target
        goal_1 = gate_mid
        goal_2 = gate_mid - centre_of_gate_to_target
        self.gate_goals = [goal_0, goal_1, goal_2]

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Planning Loop Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def get_honing_goal(self) -> tuple:
        """
        Calculates the current goal as either the "original" goal, or the average vector of a bunch of AR tags
        """
        # if we have goals to find, and we have found those goals, hone into the goal
        if self.ar_tag_manager.found_current_goals() and len(self.ar_tag_manager.ar_tag_goals) > 0:
            if len(self.ar_tag_manager.ar_tag_goals) == 1:
                return self.ar_tag_manager.get_average_goal_pose()

        # If we weren't looking for or haven't found ar tags, go to the original goal
        else:
            # If we have found ar tags, go to their average position
            return self.state_current_planning_destination

    def get_search_goal(self) -> tuple:
        """
        Returns the current search plan way-point and increments the counter
        """
        return self.search_plan[self.search_array_index]

    def get_gate_goal(self) -> tuple:
        """
        Calculates a point some distance through the gate. Assumes we are already
        at the gate, and also that we are facing the direction
        """
        assert self.ar_tag_manager.num_tags_found() == 2 and "Tried to path through a gate without two points!"

        return self.gate_goals[self.gate_array_index]

    def get_return_goal(self) -> tuple:
        return self.return_goal

    def send_next_goal(self):
        """
        Function to be called on the goal publisher timer
        """
        # update planning mode state - this is the only time in the codebase this function is called
        self.planning_mode_state_transition()

        if self.state.state == PlanningState.SUCCESS:
            self.get_logger().debug("In SUCCESS state, no planning required.")
            return
        else:
            self.get_logger().debug("plan() state is {}".format(self.state.state))

        planning_destination = Point2D()
        # polymorphism and ~functional~ programming to get the planner for the particular state

        self.get_logger().debug(
            f"Calling planner {self.planners[self.state.state]} for state {self.state.state}"
        )

        # self.get_logger().info(str(self.state.state) + " | " +  str(self.planners))
        planning_destination.x, planning_destination.y = self.planners[self.state.state]()

        # update search array index        
        if self.state.state == PlanningState.SEARCH:
            if self.search_array_index % 2 == 0 \
                    and self.search_array_index // 2 >= self.spin_counter \
                    and self.driving_state == DrivingState.TO_WAYPOINT:
                self.driving_state = DrivingState.TURNING
                self.spin_counter += 1
                self.ctl_spin = SpinController(self.state_rover_pose.yaw, self.ctl_turner)
            if self.near_current_goal():
                self.search_array_index += 1

        elif self.state.state == PlanningState.GATE_HONING:
            if self.near_current_goal():
                self.gate_array_index += 1

        self.state_current_planning_destination = (planning_destination.x, planning_destination.y)
        self.pub_desired_destination.publish(planning_destination)


def main(args=None):
    rclpy.init(args=args)
    controller = Controller()
    rclpy.spin(controller)
    controller.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
