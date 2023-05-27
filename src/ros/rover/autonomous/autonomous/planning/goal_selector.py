#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
State machine that manages selection of goals to
be sent to the path planner.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: Controller
TOPICS:
        - /autonomous/goals [AutonomousGoal]
SERVICES:
        - /autonomous/success [Trigger]
        - /autonomous/start [Trigger]
CLIENTS:
        - /autonomous/return [Trigger]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        autonomous
AUTHOR(S):      Max Tory, Liam Whittle, Liam Roy,
                Taaj Street, Aarushi Raheja,
                Niko Verrios
CREATION:       07/12/2021
EDITED:         22/04/2023
TODO:
    - Update autonomous_controller
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# ros import
import rclpy
from rclpy.node import Node
from rclpy.time import Time
from rclpy.duration import Duration
from rclpy.logging import LoggingSeverity
from rclpy.task import Future
from tf2_ros import Buffer, TransformListener

# custom message imports
from core.msg import AutonomousGoal, AutonomousGoalArray, AlvarMarkers
from geometry_msgs.msg import Transform, PoseStamped
from std_msgs.msg import Empty, Float64
from std_srvs.srv import Trigger

# autonomous imports
from autonomous.math_utils.controller_math import distance, interpolate_circle_points
from autonomous.config.ros_config import auto_goal_topic, planning_destination_topic
from autonomous.planning.ar_tag_manager import ArTagManager

# misc
from enum import Enum
import numpy as np


class PlanningState(Enum):
    IDLE = 0  # awaiting a goal
    TO_COORDINATE = 1  # honing in on a GPS coordinate
    SEARCH_SPIN = 2  # searching for an AR tag
    SEARCH = 3  # searching for an AR tag
    TO_AR_TAG = 4  # honing in on an AR tag
    THROUGH_GATE = 5  # passing through a gate
    SUCCESS = 6  # achieved goal and awaiting new instructions
    RETURN = 7  # returning autonomously to the last goal
    FAILED = 8  # failed to reach goal - currently unusued


class Controller(Node):
    """
    State machine that determines the goals to be sent to the path planner at each point in time.
    Every state transition corresponds to a change in state_current_goal, indicating that we are now planning
    to a different location. The state_long_term_goal is the coordinate we provide manually to the rover.
    Receives updates about the current pose of the rover in order to determine when goals have been achieved.
    Triggers the changing of LED colours along with state transitions.
    State transition diagram at: tinyurl.com/m2uaeycx 
    """

    def __init__(self):
        super().__init__('goal_selector')

        # set debug to not get shown
        self.get_logger().set_level(LoggingSeverity.INFO)

        # Params
        self.param_plan_frequency = self.declare_parameter("plan_frequency", 2.0).value
        self.param_ar_tag_update_frequency = self.declare_parameter("ar_tag_update_frequency", 10.0).value
        self.param_coordinate_tolerance = self.declare_parameter("coordinate_tolerance_m", 0.5).value
        self.param_ar_tag_tolerance = self.declare_parameter("ar_tag_tolerance_m", 1.3).value
        self.param_gate_goal_tolerance = self.declare_parameter("gate_tolerance_m", 0.3).value
        self.param_return_goal_tolerance = self.declare_parameter("return_tolerance_m", 5.0).value
        self.param_dist_through_gate = self.declare_parameter("dist_through_gate_m", 2.0).value

        # ~~~~~~~~~~ State ~~~~~~~~
        self.state: PlanningState = None
        self.state_current_goal : AutonomousGoal = None
        self.state_long_term_goal : AutonomousGoal = None
        # Initial return goal at 0, 0
        self.state_return_goal : AutonomousGoal = None
        self.state_ar_tag_manager = ArTagManager()
        self.state_gate_relative_vectors = None
        self.state_latest_tag_mean_pose = None
        self.state_gate_index = -1
        
        # Trigger variables set by ros callbacks
        self.trigger_received_goal = False
        self.trigger_achieved_goal = False
        self.trigger_completed_spin = False
        self.trigger_return = False

        # Arrays for storing intermediate goals
        self.state_visited_intermediate_goals = []
        self.state_unvisited_intermediate_goals = []
        self.state_search_goals = []

        # ------------- ROS Things ----------
        # tf2 buffer and listener
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(buffer=self.tf_buffer, node=self, spin_thread=True)

        # Publishers
        # Planned destination -> we wish to go here, which is the next step on our path to the target
        self.pub_desired_destination = self.create_publisher(PoseStamped, planning_destination_topic, 10)
        self.pub_do_spin = self.create_publisher(Empty, "/autonomous_controller/do_spin", 10)
        self.pub_success = self.create_publisher(Empty, "/autonomous_controller/success_trigger", 10)
        self.pub_goal_dist = self.create_publisher(Float64, "/autonomous/goal_dist", 10)

        # Subscribers
        self.sub_controller_goal_override = self.create_subscription(Empty, "/autonomous_controller/goal_achieved", self.callback_controller_goal_override, 10)
        self.sub_spin_completed = self.create_subscription(Empty, "/autonomous_controller/spin_achieved", self.callback_spin_completed, 10)
        self.sub_return = self.create_subscription(Empty, "/autonomous/return", self.callback_return, 10)
        self.sub_autonomous_goal = self.create_subscription(AutonomousGoalArray, auto_goal_topic,
                                                            self.callback_new_autonomous_goal, 10)
        self.sub_ar_tags = self.create_subscription(AlvarMarkers, "/ar_tracker/tags", self.callback_ar_tag, 10)

        # service for changing the LED
        self.srv_led_success = self.create_client(Trigger, "/autonomous/success")
        self.srv_led_start = self.create_client(Trigger, "/autonomous/start")


        self.get_logger().info("Waiting for transform from 'local_map' to 'base_link'...")
        self.transform_future : Future = self.tf_buffer.wait_for_transform_async('base_link', 'local_map', Time())
        self.transform_future.add_done_callback(self.callback_transform_received)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ State Transition Helper Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def intermediate_goal(self) -> bool:
        """
        Returns whether our current goal is an intermediate goal
        """
        return self.state_current_goal.type == AutonomousGoal.GOAL_TYPE_INTERMEDIATE

    def dist_to_point(self, point) -> float:
        """
        Returns the distance between teh current position and a given point, assumed
        to be in the global map frame
        """
        try:
            current_pose : Transform = self.tf_buffer.lookup_transform("map", "base_link", Time(), Duration(nanoseconds=1e8)).transform
        except Exception as e:
            self.get_logger().warn(f"Error in obtaining current pose: {e}")
            return -1

        self.get_logger().debug("Current position: " + str(current_pose.translation))
        self.get_logger().debug("other position: " + str(point))
        
        return distance([current_pose.translation.x, current_pose.translation.y], point) 

    def dist_to_goal(self, goal: AutonomousGoal) -> float:
        """
        Returns the distance between the current position (obtained from tf2) and a given goal.
        Assumes the goal is given in the global map frame
        """
        goal_coord = [goal.position.x, goal.position.y]
        return self.dist_to_point(goal_coord)

    def has_search_goals(self) -> bool:
        """
        Returns whether there are any search goals left to visit
        """
        return len(self.state_search_goals) > 0
    
    def at_current_goal(self) -> bool:
        """
        Function determines if we are near the final goal of this planning state. 
        This is not responsible for determining when we are approaching intermediate goals,
        as may be the case during a search plan or while passing through a gate.
        :return: True if we are near goal (and goal exists), False otherwise
        """
        # If we don't have a goal, we can't be at it
        if self.state_current_goal is None:
            self.get_logger().debug("No current goal in at_current_goal()")
            return False
        
        # controller told us to skip this goal because it couldn't get to it
        if self.state != PlanningState.SEARCH_SPIN and self.trigger_achieved_goal:
            self.get_logger().debug("Triggered achieved goal")
            return True

        # If we haven't received an override from the controller, we haven't completed a spin
        if self.state == PlanningState.SEARCH_SPIN:
            self.get_logger().debug(f"Triggered completed spin: {self.trigger_completed_spin}")
            return self.trigger_completed_spin

        dist_to_goal = self.dist_to_goal(self.state_current_goal)

        self.get_logger().debug(f"Distance to goal: {dist_to_goal}, in at_current_goal()")

        # Couldn't calculate the distance to the goal - return False
        if dist_to_goal == -1:
            return False
                                
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
        tinyurl.com/m2uaeycx 
        """
        # On startup, set to IDLE
        if self.state is None:
            self.on_state_update(PlanningState.IDLE)

        # Transition to TO_COORDINATE whenever we receive a goal, regardless of what we are doing
        if self.trigger_received_goal:
            self.on_state_update(PlanningState.TO_COORDINATE)

        # In RETURN state, rover follows intermediate goals back to previous goal. If an intermediate goal 
        # is reached then re-initialize RETURN. If previous goal is reached then transition to IDLE  
        elif self.state == PlanningState.RETURN:
            if self.at_current_goal() and self.intermediate_goal():
                self.on_state_update(PlanningState.RETURN)
            if self.at_current_goal() and not self.intermediate_goal():
                self.on_state_update(PlanningState.IDLE)

        # In TO_COORDINATE state, rover follows intermediate goals towards new goal. If an intermediate goal 
        # is reached then re-initialize TO_COORDINATE. If the individual tag or gate tags are located, then
        # transition to TO_AR_TAG or THROUGH_GATE. If new goal is reached without locating tags, transition to SEARCH.
        # If new goal is reached and we have no tags to search for, transition to SUCCESS.
        elif self.state == PlanningState.TO_COORDINATE:
            if self.trigger_return:
                self.on_state_update(PlanningState.RETURN)
            elif self.at_current_goal() and self.intermediate_goal():
                self.on_state_update(PlanningState.TO_COORDINATE)
            elif not self.intermediate_goal() and self.state_ar_tag_manager.found_current_tag():
                self.on_state_update(PlanningState.TO_AR_TAG)
            elif not self.intermediate_goal() and self.state_ar_tag_manager.found_current_gate():
                self.on_state_update(PlanningState.THROUGH_GATE)
            elif self.at_current_goal() and not self.intermediate_goal() and not self.state_ar_tag_manager.has_ar_tags():
                self.on_state_update(PlanningState.SUCCESS)
            elif self.at_current_goal() and not self.intermediate_goal() and not \
                (self.state_ar_tag_manager.found_current_tag() or self.state_ar_tag_manager.found_current_gate()):
                self.on_state_update(PlanningState.SEARCH_SPIN)

        # Transition from SEARCH_SPIN to TO_AR_TAG or THROUGH_GATE if individual tag or gate tags are located.
        # Transition to SEARCH if we have completed the spin
        elif self.state == PlanningState.SEARCH_SPIN:
            if self.trigger_return:
                self.on_state_update(PlanningState.RETURN)
            elif self.state_ar_tag_manager.found_current_tag():
                self.on_state_update(PlanningState.TO_AR_TAG)
            elif self.state_ar_tag_manager.found_current_gate():
                self.on_state_update(PlanningState.THROUGH_GATE)
            elif self.at_current_goal() and self.has_search_goals():
                self.on_state_update(PlanningState.SEARCH)
            elif self.at_current_goal() and not self.has_search_goals():
                self.on_state_update(PlanningState.FAILED)

        # Transition from SEARCH to TO_AR_TAG or THROUGH_GATE if individual tag or gate tags are located.
        # Transition to SEARCH_SPIN if we have reached the next intermediate goal
        elif self.state == PlanningState.SEARCH:
            if self.trigger_return:
                self.on_state_update(PlanningState.RETURN)
            elif self.state_ar_tag_manager.found_current_tag():
                self.on_state_update(PlanningState.TO_AR_TAG)
            elif self.state_ar_tag_manager.found_current_gate():
                self.on_state_update(PlanningState.THROUGH_GATE)
            elif self.at_current_goal():
                self.on_state_update(PlanningState.SEARCH_SPIN)

        # Transition from TO_AR_TAG to SUCCESS if reached the tag goal
        elif self.state == PlanningState.TO_AR_TAG:
            if self.trigger_return:
                self.on_state_update(PlanningState.RETURN)
            elif self.at_current_goal():
                self.on_state_update(PlanningState.SUCCESS)

        # Transition from THROUGH_GATE to SUCCESS if reached the gate goal
        elif self.state == PlanningState.THROUGH_GATE:
            if self.trigger_return:
                self.on_state_update(PlanningState.RETURN)
            elif self.at_current_goal() and self.intermediate_goal():
                self.on_state_update(PlanningState.THROUGH_GATE)
            elif self.at_current_goal() and not self.intermediate_goal():
                self.on_state_update(PlanningState.SUCCESS)
     
        # Once we have failed, we will return or wait for another goal
        elif self.state == PlanningState.FAILED:
            if self.trigger_return:
                self.on_state_update(PlanningState.RETURN)

    def on_state_update(self, new_state: PlanningState):
        """
        :param new_state: PlanningState
        Performs a number of internal downstream state updates in response to a planning update
        Most importantly, this is responsible for setting or changing the state variable "state_current_goal" at every state update
        """
        # Do the state update
        old_state = self.state
        self.get_logger().info(f"------ State Transition: {old_state} -> {new_state}")
        self.get_logger().debug(f"Before transition:\n"
                                f"Current Goal: {self.state_current_goal}\n"
                                f"Return Goal: {self.state_return_goal}\n"
                                f"Visited Intermediate Goals: {self.state_visited_intermediate_goals}\n"
                                f"Unvisited Intermediate Goals: {self.state_unvisited_intermediate_goals}\n"
                                f"Gate Goals: {self.state_gate_index}\n"
                                f"Search Goals: {self.state_search_goals}\n"
                                )
        self.state = new_state

        # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~   Update State on Transition   ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

        # Entering success state means we have reached our goal, set the LED color to Green and current goal to None,
        # and set the return goal to the goal we just reached, so we can get back there if we screw up later
        if new_state == PlanningState.SUCCESS:
            trigger = Trigger.Request()
            self.srv_led_success.call_async(trigger)
            self.pub_success.publish(Empty())
            self.state_current_goal = None
            self.state_visited_intermediate_goals = []
            self.state_unvisited_intermediate_goals = []
            self.state_gate_index = -1
            self.state_gate_relative_vectors = None

        # We aren't doing anything - set current goal to None, and stop driving
        elif new_state == PlanningState.IDLE:
            self.state_current_goal = None
            self.pub_success.publish(Empty())

        # We are driving to a coordinate - set the LED red if it isn't already, and if this is an intermediate goal, get the next goal
        elif new_state == PlanningState.TO_COORDINATE:
            if old_state == PlanningState.TO_COORDINATE:
                # We reached an intermediate goal, add it to the list of visited goals
                self.state_visited_intermediate_goals.append(self.state_current_goal)
            elif old_state == PlanningState.IDLE or old_state == PlanningState.SUCCESS:
                # We should return to our current position if we get lost
                self.state_return_goal = self.get_current_pose_as_goal()
                # We are starting a new goal, set the LED to flash red
                trigger = Trigger.Request()
                self.srv_led_start.call_async(trigger)
            # If we have more intermediate goals, set the next one as the current goal
            if len(self.state_unvisited_intermediate_goals) > 0:
                self.state_current_goal = self.state_unvisited_intermediate_goals.pop(0)
            # Otherwise, we will now go to the final goal
            else:
                self.state_current_goal = self.state_long_term_goal
        
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

        # We are going through a gate - set up the gate goals if we haven't already, then go to the next one in the queue
        elif new_state == PlanningState.THROUGH_GATE:
            if old_state != PlanningState.THROUGH_GATE:
                self.set_gate_goals()
                self.state_gate_index = 0
            else:
                self.state_gate_index += 1
            self.state_current_goal = self.get_gate_goal()
            
        # We are spinning in place to search for a tag or gate, no need to set current goal since this is never used
        elif new_state == PlanningState.SEARCH_SPIN:
            if not old_state == PlanningState.SEARCH:
                self.set_search_goals()
            self.pub_do_spin.publish(Empty())
        
        # We are driving to the next coordinate in the search plan
        elif new_state == PlanningState.SEARCH:
            self.state_current_goal = self.state_search_goals.pop(0)

        elif new_state == PlanningState.FAILED:
            self.get_logger().error("FAILED TO REACH GOAL")
            self.pub_success.publish(Empty())
            self.state_current_goal = None

        # Reset trigger state variables
        self.trigger_return = False
        self.trigger_achieved_goal = False
        self.trigger_received_goal = False
        self.trigger_completed_spin = False

        self.get_logger().debug(f"After transition:\n"
                                f"Current Goal: {self.state_current_goal}\n"
                                f"Return Goal: {self.state_return_goal}\n"
                                f"Visited Intermediate Goals: {self.state_visited_intermediate_goals}\n"
                                f"Unvisited Intermediate Goals: {self.state_unvisited_intermediate_goals}\n"
                                f"Gate index: {self.state_gate_index}\n"
                                f"Search Goals: {self.state_search_goals}\n"
                                )

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ROS callbacks ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def callback_transform_received(self, future: Future):
        """
        Called when we have received a transform from the tf buffer
        """
        try:
            future.result()
        except Exception as e:
            self.get_logger().error(f"Failed to get transform: {e}")
            self.destroy_node()
        else:
            self.get_logger().info("Received Transform!")

            # Timers
            self.timer_planning = self.create_timer(1 / self.param_plan_frequency, self.send_next_goal)  # update planning state and plan paths
            self.timer_ar_tag_update = self.create_timer(1 / self.param_ar_tag_update_frequency, self.update_ar_tags)  # update planning state and plan paths
            self.on_state_update(PlanningState.IDLE)

    def callback_new_autonomous_goal(self, msg : AutonomousGoalArray):
        """
        Callback for autonomous_goal topic. Called when we have received a new goal from the planner with its
        associated intermediate path goals
        """
        self.state_ar_tag_manager.set_goal(msg.goals[-1])
        self.state_long_term_goal = msg.goals[-1]
        self.state_unvisited_intermediate_goals = msg.goals[:-1]
        self.trigger_received_goal = True
    
    def callback_ar_tag(self, msg: AlvarMarkers):
        """
        :param msg: AlvarMarker msg type, received from the ar_tag_topic
        """
        try:
            depth_cam_transform : Transform = self.tf_buffer.lookup_transform("map", msg.header.frame_id, Time.from_msg(msg.header.stamp), Duration(nanoseconds=1e8))
        except Exception as e:
            self.get_logger().warn(f"Failed to find depth camera transform: {e}", throttle_duration_sec=1)
            return
        self.state_ar_tag_manager.update_tags(msg, depth_cam_transform)

    def callback_controller_goal_override(self, msg):
        """
        The controller has indicated that it can't get any closer to this goal
        """
        self.trigger_achieved_goal = True

    def callback_spin_completed(self, msg):
        """
        The controller has indicated that it has finished spinning
        """
        self.trigger_completed_spin = True

    def callback_return(self, msg):
        """
        We have received a return message over ROS
        """
        self.trigger_return = True

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Special Goal Helper Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def set_search_goals(self):
        """
        Constructs a list of AutonomousGoal objects of type INTERMEDIATE in a circle around the location we expect
        to find a tag or gate
        """
        goal_coord = [self.state_current_goal.position.x, self.state_current_goal.position.y]
        search_plan_coords = interpolate_circle_points(goal_coord)
        self.state_search_goals = []
        for x, y in search_plan_coords:
            search_goal = AutonomousGoal()
            search_goal.position.x = x
            search_goal.position.y = y
            search_goal.type = AutonomousGoal.GOAL_TYPE_INTERMEDIATE
            self.state_search_goals.append(search_goal)

    def get_current_pose_as_goal(self) -> AutonomousGoal:
        """
        Returns the current position of the rover as an AutonomousGoal of type GOAL_TYPE_COORDINATE
        """
        try:
            current_pose : Transform = self.tf_buffer.lookup_transform("map", "base_link", Time(), Duration(nanoseconds=1e8)).transform
        except Exception as e:
            self.get_logger().warn(f"Error in at_current_goal: {e}")
            return None
        goal = AutonomousGoal()
        goal.position.x, goal.position.y = current_pose.translation.x, current_pose.translation.y
        goal.type = AutonomousGoal.GOAL_TYPE_COORDINATE
        return goal

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Planning Loop ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def send_next_goal(self):
        """
        Function to be called on the goal publisher timer
        """
        # update planning mode state - this is the only time in the codebase this function is called
        self.planning_mode_state_transition()

        if self.state in [PlanningState.SUCCESS, PlanningState.IDLE, PlanningState.FAILED, PlanningState.SEARCH_SPIN]:
            self.get_logger().debug("No planning required.")
            return
        else:
            self.get_logger().debug(f"plan state is {self.state}")
        
        if self.state == PlanningState.TO_AR_TAG:
            self.state_current_goal = self.get_ar_tag_goal()
        elif self.state == PlanningState.THROUGH_GATE:
            self.state_current_goal = self.get_gate_goal()

        dist_to_goal = self.dist_to_goal(self.state_long_term_goal)
        self.pub_goal_dist.publish(Float64(data=dist_to_goal))
        planning_destination = PoseStamped()
        planning_destination.header.stamp = self.get_clock().now().to_msg()
        planning_destination.header.frame_id = "map"

        planning_destination.pose.position.x = self.state_current_goal.position.x
        planning_destination.pose.position.y = self.state_current_goal.position.y

        self.pub_desired_destination.publish(planning_destination)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ AR tag methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def set_gate_goals(self):
        """
        Gets the midpoint of the gate and the vector perpendicular to the gate. Then constructs a list of three
        AutonomousGoal objects, on the closer side of the gate, in the middle of the gate, and on the further side of the gate.
        The first two goals must be of type GOAL_TYPE_INTERMEDIATE, and the last of type GOAL_TYPE_GATE 
        """
        gate_centre = self.state_ar_tag_manager.get_average_goal_pose()
        
        # Vector perpendicular to the vector between the two gate points
        gate_perpendicular = self.state_ar_tag_manager.get_gate_normal()

        # Vector from the middle of the two gate poles to our target
        centre_of_gate_to_target = self.param_dist_through_gate * gate_perpendicular
        goal_0 = centre_of_gate_to_target
        goal_1 = np.array([0.0, 0.0])
        goal_2 = -centre_of_gate_to_target

        goal_coords = [goal_0, goal_1, goal_2]

        dist_1 = self.dist_to_point(goal_coords[0] + gate_centre) 
        dist_2 = self.dist_to_point(goal_coords[2] + gate_centre)

        if dist_1 == -1 or dist_2 == -1:
            self.get_logger().warn("Not checking gate goal orientation")
        
        elif dist_1 > dist_2:
            # We are closer to the last goal than the first, so we should reverse the order
            goal_coords = goal_coords[::-1]

        self.state_gate_relative_vectors = goal_coords

    def coord_to_goal(self, coord, type) -> AutonomousGoal:
        """
        Utility method that creates an AutonomousGoal type from a coordinate
        """
        x, y = coord[:2]
        goal = AutonomousGoal()
        goal.position.x = x
        goal.position.y = y
        goal.type = type
        return goal

    def get_ar_tag_goal(self) -> AutonomousGoal:
        """
        Returns the averaged position of the current targeted AR tag as a goal
        """
        tag_coord = self.state_ar_tag_manager.get_average_goal_pose()
        return self.coord_to_goal(tag_coord, type=AutonomousGoal.GOAL_TYPE_TAG)
    
    def get_gate_goal(self) -> AutonomousGoal:
        """
        Assumes we are going to a gate and have already called self.set_gate_goals()
        """
        if self.state_gate_relative_vectors is None:
            self.get_logger().error("Cannot get gate goal without initialising gate vectors!!")
            return
        gate_centre_coord = self.state_ar_tag_manager.get_average_goal_pose()
        gate_poses = [np.array(gate_centre_coord) + offset for offset in self.state_gate_relative_vectors]
        gate_goals = [self.coord_to_goal(pose, type=AutonomousGoal.GOAL_TYPE_INTERMEDIATE) for pose in gate_poses]
        gate_goals[-1].type = AutonomousGoal.GOAL_TYPE_GATE
        return gate_goals[self.state_gate_index]

    def update_ar_tags(self):
        """
        Spin the AR tag tracker and check if we know the latest mean pose
        """
        rclpy.spin_once(self.state_ar_tag_manager)
        if self.state_ar_tag_manager.has_goal():
            self.state_latest_tag_mean_pose = self.state_ar_tag_manager.get_average_goal_pose()


def main(args=None):
    rclpy.init(args=args)
    controller = Controller()
    rclpy.spin(controller)
    controller.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
