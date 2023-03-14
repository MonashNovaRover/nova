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
from core.msg import DriveInput, RoverPose, Waypoints, AlvarMarker, AutonomousGoal, PivotWheelData
from controller.ar_tag_manager import ArTagManager
from controller.spin_controller import SpinController

# autonomous imports
from autonomous.math_utils.controller_math import *
import autonomous.math_utils.transform as transform
from autonomous.config.runtime_params import *
from autonomous.config.ros_config import *
from autonomous.planning.path_planner import PathPlanner
from autonomous.controller.search_manager import SearchManager
from autonomous.controller.drive_controller import DriveController
import autonomous.config as config

# misc
from enum import Enum
from typing import List, Tuple
import logging


class GoalType(Enum):
    NO_GOAL = 0
    GPS = 1
    GPS_TAG = 2
    GPS_GATE = 3


class DrivingState(Enum):
    TURNING = 0  # doing a 360-degree turn on the spot
    TO_WAYPOINT = 1  # driving to the next waypoint in a path


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

        # warn user if state is being loaded from disk
        # self.saved_state_fn = "saved_state.txt"
        # if self.saved_state_fn in listdir("."):
        #    self.get_logger().warn("State will initialise from saved state as " + str(self.state))

        # if self.saved_state_fn in listdir("."):
        #    with open(self.saved_state_fn, "r") as fp:
        #        self.state = PlanningState(int(fp.read()))

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
        self.get_logger().set_level(logging.INFO)

        # Ros params
        self.param_is_arc = self.declare_parameter("is_ARC", True).value

        # ~~~~~~~~~~ State ~~~~~~~~
        self.state_rover_pose = Pose2D()
        if not self.param_is_arc:
            self.ar_tag_manager = ArTagManager()
        else:
            self.search_manager : SearchManager = SearchManager()
        self.planning_state = SavedPlanningState(logger=self.get_logger())
        self.waypoint_path = []
        self.state_current_planning_destination = None
        self.driving_state = DrivingState.TO_WAYPOINT
        self.spin_counter = 0
        self.var_latest_steer = 0

        # Global variables containing our search plan
        self.search_plan = []
        self.search_array_index = 0

        # these are the planners we use when in each particular state
        self.planners = {
            PlanningState.GPS_HONING: self.get_honing_goal,
            PlanningState.AR_HONING: self.get_honing_goal,
            PlanningState.SEARCH: self.get_search_goal,
            PlanningState.GATE: self.get_gate_goal,
            # cheeky lambda 
            PlanningState.SUCCESS: None # lambda: self.get_logger().fatal("Why is SUCCESS being called?")
        }

        # Controller classes for turning, driving to waypoints, and spinning
        self.ctl_driver = DriveController()
        self.ctl_spin = None

        # Node is initialised, begin search (If ARC)
        if self.param_is_arc:
            self.on_state_update(PlanningState.SEARCH)

        # ------------- ROS Things ----------
        # tf2
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(buffer=self.tf_buffer, node=self, spin_thread=True)

        # Publishers
        # 'DriveInput' message is used to make the wheels move!
        self.pub_drive_commands = self.create_publisher(DriveInput, auto_drive_command_topic, 10)
        # Planned destination -> we wish to go here, which is the next step on our path to the target
        self.pub_desired_destination = self.create_publisher(PoseStamped, planning_destination_topic, 10)

        # Subscribers
        self.sub_ar_tags = self.create_subscription(AlvarMarker, ar_track_topic, self.callback_ar_tag, 10)
        self.sub_autonomous_goal = self.create_subscription(AutonomousGoal, auto_goal_topic,
                                                            self.callback_new_autonomous_goal, 10)
        self.sub_planned_path_to_destination = self.create_subscription(Waypoints, auto_waypoints_topic,
                                                                        self.callback_planner_path, 10)
        self.sub_steer = self.create_subscription(PivotWheelData, "/control/wheel_pivots", self.callback_steer, 10)
        # service for changing the LED
        #self.srv_led_success = self.create_client(Trigger, "/autonomous/success")
        #self.srv_led_start = self.create_client(Trigger, "/autonomous/start")

        # Timers
        self.control_timer = self.create_timer(0.1, self.control)  # calculate and send drive commands
        self.planning_timer = self.create_timer(0.5, self.plan)  # update planning state and plan paths
        self.pose_timer = self.create_timer(0.1, self.callback_rover_pose)  # update the rover's pose from tf2


    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ State Transition Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def reset_goals_and_waypoints(self):
        """
        sets all original goals, best effort goals and stored paths back to default state
        """
        # original goal could be a GPS coordinates (in local frame),
        # AR tags, gate based goals, or search goals
        self.waypoint_path = []
        self.state_current_planning_destination = None
        self.driving_state = DrivingState.TO_WAYPOINT
        self.search_array_index = 0

    def planning_mode_state_transition(self):
        """
        Update current planning mode based on the rest of the state
        """

        # note: SUCCESS mode is the only planning mode which required (or can be changed by)
        # an edge triggered change - the only place this happens is in self.callback_new_autonomous_goal()
        if self.planning_state.state == PlanningState.SUCCESS:
            self.get_logger().debug("In success mode, waiting for edge triggered update to GPS honing", throttle_duration_sec=1)

        elif self.planning_state.state == PlanningState.GPS_HONING:
            if self.near_current_goal() and len(self.ar_tag_manager.ar_tag_goals) == 0:
                self.on_state_update(PlanningState.SUCCESS)

            elif self.near_current_goal() and not self.ar_tag_manager.found_current_goals() \
                    and len(self.ar_tag_manager.ar_tag_goals) != 0:
                self.on_state_update(PlanningState.SEARCH)

            elif self.ar_tag_manager.found_current_goals() and len(self.ar_tag_manager.ar_tag_goals) != 0:
                self.on_state_update(PlanningState.AR_HONING)

        elif self.planning_state.state == PlanningState.SEARCH:

            if self.param_is_arc:
                if self.search_manager.search_complete():
                    self.on_state_update(PlanningState.SUCCESS)
            else:
                # At URC, only search until we have found the AR tags for this goal
                if self.ar_tag_manager.found_current_goals():
                    self.on_state_update(PlanningState.AR_HONING)
                elif self.search_array_index == len(self.search_plan):
                    self.on_state_update(PlanningState.SUCCESS)

        elif self.planning_state.state == PlanningState.AR_HONING:

            if self.near_current_goal() and len(self.ar_tag_manager.ar_tag_goals) == 2:
                self.on_state_update(PlanningState.GATE)

            elif self.near_current_goal():
                self.on_state_update(PlanningState.SUCCESS)

        elif self.planning_state.state == PlanningState.GATE:

            if self.near_current_goal():
                self.on_state_update(PlanningState.SUCCESS)

    def on_state_update(self, new_state: PlanningState):
        """
        :param new_state: PlanningState
        Performs a number of internal downstream state updates in response to a planning update
        """
        old_state = self.planning_state.state
        self.get_logger().info("------ State Transition: " + str(old_state) + " -> " + str(new_state))
        self.planning_state.update_state(new_state)
        self.driving_state = DrivingState.TO_WAYPOINT
        if new_state == PlanningState.SUCCESS:
            trigger = Trigger.Request()
            #self.srv_led_success.call_async(trigger)

        if old_state == PlanningState.SUCCESS:
            trigger = Trigger.Request()
            #self.srv_led_start.call_async(trigger)
            
        if new_state == PlanningState.SEARCH:
            self.setup_search()

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Simple State Update Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def get_current_goal_type(self):
        """
        The goal provided is in update_goal, contains a position and also a number of AR tag ID's to search for.
        Knowing this, we can determine the type of goal based on the number of AR tag ID's given.
        0 tags - precise position given, we must get there on dGPS alone
        1 tag - rough location given, we must go there and then travel to the AR tag when we find it.
        2 tags - rough location + two tags, we need to travel between the two tags as this is a GATE.
        """
        if self.state_current_planning_destination is None:
            return GoalType.NO_GOAL
        else:
            # using if/elif here for CLARITY
            tag_count = len(self.ar_tag_manager.ar_tag_goals)
            if tag_count == 0:
                return GoalType.GPS
            elif tag_count == 1:
                return GoalType.GPS_TAG
            elif tag_count == 2:
                return GoalType.GPS_GATE
            else:
                raise ValueError("Too many ar_tag_goals, should be <= 2")

    def current_goal_type_is(self, goal_type: GoalType):
        """
        :param goal_type: GoalType query to compare to self
        :return: True if current goal type is goal_type
        """
        return self.get_current_goal_type() == goal_type

    def callback_planner_path(self, msg):
        """
        The callback which is called from the path planner subscriber. We need to:
        - Update list of waypoints in way-point path
        - update best effort goal
        Note that when we are near our goal, best effort goal will be set, but way-point path won't be set.
        :param msg: Waypoints message from the path planner
        """
        self.waypoint_path = [(p.x, p.y) for p in msg.waypoints]
        # removed the below list comprehension filter as it is handled
        # in control()
        # if distance((self.state_rover_pose.x, self.state_rover_pose.y),
        # (p.x, p.y)) > min_waypoint_distance]
        self.planning_state.num_paths_planned += 1

    def callback_rover_pose(self):
        """
        Stores the latest rover pose message into our State() variable
        """
        try:
            base_link_tf : Transform = self.tf_buffer.lookup_transform("local_map", "base_link", Time()).transform
            self.get_logger().debug("Found transform from local_map to base_link", once=True, throttle_duration_sec=1)
        except:
            self.get_logger().warn("No transform from local_map to base_link", once=True)
        else:
            self.state_rover_pose.x = base_link_tf.translation.x
            self.state_rover_pose.y = base_link_tf.translation.y
            self.state_rover_pose.yaw = transform.quat_to_euler(base_link_tf.rotation)[2]

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
        self.get_logger().debug(f"Received new goal: {msg}", throttle_duration_sec=1)
        self.reset_goals_and_waypoints()
        self.ar_tag_manager.ar_tag_goals = [iD for iD in msg.ids]
        self.state_current_planning_destination = msg.position.x, msg.position.y
        # WARNING: edge triggered state update outside
        self.on_state_update(PlanningState.GPS_HONING)

    def callback_ar_tag(self, msg):
        """
        :param msg: AlvarMarker msg type, received from the ar_tag_topic
        """
        # we pass in our current state so the tag manager knows how to transfer the AR tag pose to a global frame
        self.ar_tag_manager.update_tags(msg, self.state_rover_pose, self.get_logger())

    def callback_steer(self, msg):
        """
        :param msg: WheelPivotData
        """
        self.var_latest_steer = msg

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 'Util' Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def setup_search(self):
        if self.param_is_arc:
            self.search_plan = self.load_search_layout()
        else:
            self.search_plan = interpolate_circle_points(self.state_current_planning_destination)
        self.ctl_spin = SpinController(self.state_rover_pose.yaw, self.ctl_driver)

    def load_search_layout(self) -> List[Tuple[float, float]]:
        """
        Get search as a list of (x, y) coordinates from the parameter list
        """
        return np.array(self.param_search_coordinates).reshape(-1, 2)

    def near_current_goal(self) -> bool:
        """
        Function determines (if there is a goal) if we are near the goal. If there is no
        planned goal or state_current_planning_destination, we return False, since we 
        must have a goal to be near it. 
        :return: Boolean value of True if we are near goal (and goal exists), False otherwise
        """

        # look for a best effort goal, else compare to an original goal
        if self.planning_state.num_paths_planned == 0:
            return False

        end_of_path = self.waypoint_path[-1] if len(self.waypoint_path) > 0 else None
        if end_of_path is None:
            return True  # path is only empty if
        return distance(
            np.array([self.state_rover_pose.x, self.state_rover_pose.y]),
            end_of_path
        ) < min_waypoint_distance

    def prune_waypoints(self):
        """
        Return a sub-list of the planning waypoints containing only those at least some minimum distance from
        the rover
        """
        assert self.waypoint_path is not None
        return [point for point in self.waypoint_path if distance(
            (self.state_rover_pose.x, self.state_rover_pose.y),
            point
            ) > min_waypoint_distance]

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Planning Loop Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def get_honing_goal(self) -> AutonomousGoal:
        """
        Calculates the current goal as either the "original" goal, or the average vector of a bunch of AR tags
        """
        # if we have goals to find, and we have found those goals, hone into the goal
        if self.ar_tag_manager.found_current_goals() and len(self.ar_tag_manager.ar_tag_goals) > 0:
            return self.ar_tag_manager.get_average_goal_pose()

        # If we weren't looking for or haven't found ar tags, go to the original goal
        else:
            # If we have found ar tags, go to their average position
            return self.state_current_planning_destination

    def get_search_goal(self) -> AutonomousGoal:
        """
        Returns the current search plan way-point and increments the counter
        """
        return self.search_manager.get_current_goal(current_pos=[self.state_rover_pose.x, self.state_rover_pose.y])

    def get_gate_goal(self) -> AutonomousGoal:
        """
        Calculates a point some distance through the gate. Assumes we are already
        at the gate, and also that we are facing the direction
        """
        assert self.ar_tag_manager.num_tags_found() == 2 and "Tried to path through a gate without two points!"

        gate_mid = self.ar_tag_manager.get_average_goal_pose()

        # negative reciprocal gives perpendicular vector to the vector between the gate
        gate_perpendicular = self.ar_tag_manager.get_gate_normal()
        # Current yaw as vector
        orientation_vector = np.array([np.cos(self.state_rover_pose.yaw), np.sin(self.state_rover_pose.yaw)])
        # -1 if we are facing away from perpendicular vector, +1 if we are towards it
        direction = np.sign(np.dot(orientation_vector, gate_perpendicular))

        # Vector from the middle of the two gate poles to our target
        centre_of_gate_to_target = direction * dist_through_gate_m * gate_perpendicular
        goal = gate_mid + centre_of_gate_to_target
        return goal[0], goal[1]

    def plan(self):
        """
        Function to be called on the goal publisher timer
        """
        # update planning mode state - this is the only time in the codebase this function is called
        self.planning_mode_state_transition()

        if self.planning_state.state == PlanningState.SUCCESS:
            self.get_logger().debug("In SUCCESS state, no planning required.", throttle_duration_sec=1)
            return
        else:
            self.get_logger().debug("plan() state is {}".format(self.planning_state.state), throttle_duration_sec=1)

        planning_destination = PoseStamped()
        planning_destination.header.stamp = self.get_clock().now().to_msg()
        planning_destination.header.frame_id = "map"
        planning_destination.pose.orientation.w = 1.0
        # polymorphism and ~functional~ programming to get the planner for the particular state

        self.get_logger().debug(
            "Calling planner {} for state {}".format(
                self.planners[self.planning_state.state],
                self.planning_state.state
            )
        , throttle_duration_sec=1)
        # self.get_logger().info(str(self.planning_state.state) + " | " +  str(self.planners))
        auto_goal = self.planners[self.planning_state.state]()
        planning_destination.pose.position.x = auto_goal.position.x
        planning_destination.pose.position.y = auto_goal.position.y

        # update search array index        
        if self.planning_state.state == PlanningState.SEARCH:
            if auto_goal.type == AutonomousGoal.GOAL_TYPE_SPIN and self.driving_state == DrivingState.TO_WAYPOINT:
                self.driving_state = DrivingState.TURNING
                self.ctl_spin = SpinController(self.state_rover_pose.yaw, self.ctl_driver)
            if self.near_current_goal():
                if self.param_is_arc:
                    self.search_manager.at_goal()
                else:
                    self.search_array_index += 1

        self.state_current_planning_destination = (planning_destination.pose.position.x, planning_destination.pose.position.y)
        self.pub_desired_destination.publish(planning_destination)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Control Loop Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def go_to_target(self, target_waypoint: tuple):
        """
        Publishes a single drive command to navigate to the current target waypoint.
        Called every tick by the control method. Turns in place to face towards the waypoint,
        or drives towards it in a straight line. If the rover has just finished turning, a
        single zero drive command is sent before driving begins.
        :param:
        """
        # calculate target yaw and signed yaw difference using the controller_math module
        self.get_logger().debug(f"driving to {target_waypoint} from {self.state_rover_pose}", throttle_duration_sec=1)

        position_vector = np.array([self.state_rover_pose.x, self.state_rover_pose.y, 0])
        target_vector = np.array([target_waypoint[0], target_waypoint[1], 0])

        desired_orientation = target_vector - position_vector
        desired_orientation /= np.linalg.norm(desired_orientation)

        current_orientation = np.array([np.cos(self.state_rover_pose.yaw), np.sin(self.state_rover_pose.yaw), 0])

        yaw_diff = yaw_difference(current_orientation, desired_orientation)

        self.get_logger().debug(f"desired: {desired_orientation}, current: {current_orientation}, yaw_diff: {yaw_diff}", throttle_duration_sec=1)

        speed, steer = self.ctl_driver.get_drive_command(yaw_diff, self.var_latest_steer, position_vector, current_orientation)
        self.send_drive_cmd(speed, steer)

    def control(self):
        """
        Called once every tick by the node's timer. Identifies the next target waypoint
        and calls navigate_to_waypoint, and determines when the rover has arrived
        """
        if self.planning_state.state == PlanningState.SUCCESS:
            self.get_logger().debug("Controller mode: success", throttle_duration_sec=1)
            return
        if self.planning_state.num_paths_planned < 1:
            self.get_logger().debug("Not enough paths planned", throttle_duration_sec=1)
            return

        current_orientation = np.array([np.cos(self.state_rover_pose.yaw), np.sin(self.state_rover_pose.yaw), 0.])
        current_position = np.array([self.state_rover_pose.x, self.state_rover_pose.y, 0.])

        self.get_logger().debug("Controller in driving state: " + str(self.driving_state), throttle_duration_sec=1)

        # -------------------------------------- 0. TURNING ------------------------------
        if self.planning_state.state == PlanningState.SEARCH and self.driving_state == DrivingState.TURNING:
            if not self.ctl_spin.is_completed():
                drive, steer = self.ctl_spin.turn_in_place(self.var_latest_steer, current_orientation)
                self.send_drive_cmd(drive, steer)
            else:
                self.driving_state = DrivingState.TO_WAYPOINT
                self.search_manager.at_goal()

        # -------------------------------------- 1. DRIVING ------------------------------
        path = self.prune_waypoints()
        if self.driving_state == DrivingState.TO_WAYPOINT and len(path) > 0:
            # can only go to waypoints
            # Drive to a waypoint
            self.get_logger().debug("In TO_WAYPOINT controller", throttle_duration_sec=1)
            self.go_to_target(path[0])
        elif len(path) == 0:
            self.get_logger().debug("No more waypoints in path.", throttle_duration_sec=1)

    def send_drive_cmd(self, drive_fraction: float, angular_fraction: float):
        """
        Publishes drive commands to the auto_drive_commands topic
        :param: drive_fraction: [-1:1]
        :param: angular_fraction: [-1:1]
        """

        # construct message to publish
        drive_cmd_msg = DriveInput()
        # Values are validated to stay within -1:1
        drive_cmd_msg.speed = max(-1.0, min(1.0, float(drive_fraction)))
        drive_cmd_msg.steer = max(-1.0, min(1.0, float(angular_fraction)))

        # Print!
        self.get_logger().debug("Driving at speed {:.4f}, steer {:.4f}".format(
            drive_cmd_msg.speed, drive_cmd_msg.steer
        ), throttle_duration_sec=1)

        # publish to public topic
        self.pub_drive_commands.publish(drive_cmd_msg)


def handle_path_status(self, status):
    """
    Handles logging and adjusting of parameters according to the
    status returned by our c++ A* method.
    """
    if status & PathPlanner.A_STAR_START_OBSTACLE:
        self.get_logger().warn("started in obstacle")
    if status & PathPlanner.A_STAR_DEST_OBSTACLE:
        self.get_logger().warn("destination in obstacle")
    if status & PathPlanner.A_STAR_NO_PATH:
        self.get_logger().warn("couldn't find a path initially")
    if status & PathPlanner.A_STAR_CRITICAL_NO_PATH:
        self.get_logger().error("COULDN'T FIND PATH - NEAR OBSTACLE")
        self.padding_dist_m -= 0.1
        if self.padding_dist_m < 0.4:
            self.get_logger().error("Ah HECK")
            return
    if status == PathPlanner.A_STAR_SUCCESS:
        self.get_logger().info("A* found safe path")


def main(args=None):
    rclpy.init(args=args)
    controller = Controller()
    rclpy.spin(controller)
    controller.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
