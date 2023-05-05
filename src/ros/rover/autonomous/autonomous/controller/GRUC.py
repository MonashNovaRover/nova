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
from nav_msgs.msg import Path
from std_msgs.msg import Empty, Bool
from geometry_msgs.msg import Transform, Pose2D
from tf2_ros import Buffer, TransformListener
from rclpy.duration import Duration

# custom message imports
from core.msg import DriveInput, AutonomousGoal, PivotWheelData, BLCMDReset, BLCMDStatusArray, BLCMDStatus
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
import time
import numpy as np


class DrivingState(Enum):
    TURNING = 0  # doing a 360-degree turn on the spot
    TO_WAYPOINT = 1  # driving to the next waypoint in a path
    TO_TARGET = 2  # driving to a block or tag
    FACE_TARGET = 3  # turning to face a block or tag
    SUCCESS = 4  # Completed driving to the current goal
    PRE_RESET = 5  # Resetting resolvers after a fault and waiting
    POST_RESET = 6  # Resetting blcmds after wait period
    RESET_WAIT = 7 # Wait for 1 minute



class Controller(Node):
    """
    Controls the movement of the rover between waypoints determined by the path planner.
    Receives updates about the current pose of the rover and the waypoints to
    navigate between via ros topics autonomous/pose and autonomous/goals. Publishes drive
    commands to auto_drive_commands
    """

    def __init__(self):
        super().__init__('GRUC')

        # set debug to not get shown
        self.get_logger().set_level(logging.INFO)

        # Ros params
        self.param_do_tank_turn = self.declare_parameter("do_tank_turn", False).value
        self.param_dist_to_targets = self.declare_parameter("dist_to_target_m", 2.5).value
        self.param_dist_to_search_points = self.declare_parameter("dist_to_search_point_m", 2.0).value
        self.param_waypoint_follow_distance = self.declare_parameter("waypoint_follow_distance_m", 0.3).value
        self.param_goal_facing_threshold = self.declare_parameter("goal_facing_threshold_rad", np.pi/8).value
        self.param_max_goal_achieved_dist = self.declare_parameter("max_distance_to_achieve_goal_m", 4.0).value

        # ~~~~~~~~~~ State ~~~~~~~~
        self.state_rover_pose = None
        self.waypoint_path = []
        self.driving_state = DrivingState.SUCCESS
        self.var_latest_steer = 0
        self.turning_mode = TurningMode.TANK if self.param_do_tank_turn else TurningMode.PIVOT
        self.state_current_planning_destination : AutonomousGoal = None
        self.num_paths_planned = 0

        self.auto_mode = False

        # Controller classes for turning, driving to waypoints, and spinning
        self.ctl_driver = DriveController(self.turning_mode)
        self.ctl_spin = None

        # Reset Things
        self.saved_state = None
        self.reset_time = None
        self.blcmd_errors = []
        self.resolver_errors = []
        self.last_reset = None

        # ------------- ROS Things ----------
        # tf2
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(buffer=self.tf_buffer, node=self, spin_thread=True)

        # Publishers
        # 'DriveInput' message is used to make the wheels move!
        self.pub_drive_commands = self.create_publisher(DriveInput, auto_drive_command_topic, 10)
        self.pub_at_goal = self.create_publisher(Empty, "~/at_goal", 10)
        self.pub_blcmd_reset = self.create_publisher(BLCMDReset, "/control/blcmd_reset", 10)
        # Planned destination -> we wish to go here, which is the next step on our path to the target

        # Subscribers
        self.sub_autonomous_goal = self.create_subscription(AutonomousGoal, "/goal_manager/goals",
                                                            self.callback_new_autonomous_goal, 10)
        self.sub_planned_path_to_destination = self.create_subscription(Path, auto_waypoints_topic,
                                                                        self.callback_planner_path, 10)
        self.sub_steer = self.create_subscription(PivotWheelData, "/control/pivot_wheel", self.callback_steer, 10)
        self.sub_blcmd_status = self.create_subscription(BLCMDStatusArray, "/control/blcmd_status", self.callback_blcmd_status, 10)
        self.auto_mode_sub = self.create_subscription(Bool, "/autonomous/mode", self.auto_mode_callback, 10)

        self.get_logger().info("Waiting for transform from 'local_map' to 'base_link'...")
        while not self.tf_buffer.can_transform('base_link', 'map', Time()):
            time.sleep(0.1)
        self.get_logger().info("Received Transform!")

        # Timers
        self.control_timer = self.create_timer(0.1, self.control)  # calculate and send drive commands
        self.pose_timer = self.create_timer(0.1, self.callback_rover_pose)  # update the rover's pose from tf2


    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ State Transition Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def reset_goals_and_waypoints(self):
        """
        sets all original goals, best effort goals and stored paths back to default state
        """
        # original goal could be a GPS coordinates (in local frame),
        # AR tags, gate based goals, or search goals
        self.num_paths_planned = 0
        self.waypoint_path = []
        self.ctl_spin = None

    def drive_mode_state_transition(self):
        """
        Update current planning mode based on the rest of the state
        """
        current_goal = self.state_current_planning_destination
        if current_goal is None:
            if self.driving_state != DrivingState.SUCCESS:
                self.on_drive_state_update(DrivingState.SUCCESS)
        elif self.driving_state == DrivingState.SUCCESS:
            # We've just finished a waypoint, so we need to look for a new one
            if current_goal.type == AutonomousGoal.GOAL_TYPE_SPIN:
                self.on_drive_state_update(DrivingState.TURNING)
            elif current_goal.type == AutonomousGoal.GOAL_TYPE_HONING:
                if not self.near_current_goal():
                    self.on_drive_state_update(DrivingState.TO_WAYPOINT)
            elif current_goal.type in [AutonomousGoal.GOAL_TYPE_BLOCK, AutonomousGoal.GOAL_TYPE_TAG]:
                if not self.near_current_goal():
                    self.on_drive_state_update(DrivingState.TO_TARGET)
                elif not self.facing_current_goal():
                    self.on_drive_state_update(DrivingState.FACE_TARGET)
        elif self.driving_state == DrivingState.TO_WAYPOINT:
            if self.near_current_goal():
                # Stop driving to goals when we get close to them
                self.on_drive_state_update(DrivingState.SUCCESS)
        elif self.driving_state == DrivingState.TO_TARGET:
            if self.near_current_goal():
                # Stop driving to goals when we get close to them
                self.on_drive_state_update(DrivingState.FACE_TARGET)
        elif self.driving_state == DrivingState.FACE_TARGET:
            if self.facing_current_goal():
                # near and facing the current goal, so we can stop
                self.on_drive_state_update(DrivingState.SUCCESS)
        elif self.driving_state == DrivingState.TURNING:
            if self.ctl_spin.is_completed():
                # We have just finished a spin
                self.on_drive_state_update(DrivingState.SUCCESS)
        elif self.driving_state == DrivingState.PRE_RESET:
            self.get_logger().info(f'{(self.get_clock().now() - self.reset_time).nanoseconds/1e9} since entering pre-reset state', throttle_duration_sec=1)
            if (self.get_clock().now() - self.reset_time) >= Duration(seconds = 7):
                self.on_drive_state_update(DrivingState.POST_RESET)
        elif self.driving_state == DrivingState.POST_RESET:
            self.get_logger().info(
                f'{(self.get_clock().now() - self.reset_time).nanoseconds / 1e9} since entering post-reset state', throttle_duration_sec=1)
            if (self.get_clock().now() - self.reset_time) >= Duration(seconds = 3):
               state = self.saved_state
               self.saved_state = None
               self.reset_time = None
               self.on_drive_state_update(state)
        elif self.driving_state == DrivingState.RESET_WAIT:
            self.get_logger().info(
                f'{(self.get_clock().now() - self.last_reset).nanoseconds / 1e9} since last reset state', throttle_duration_sec=1)
            if (self.get_clock().now() - self.last_reset) >= Duration(seconds=10):
                self.on_drive_state_update(DrivingState.PRE_RESET)



    def on_drive_state_update(self, new_state: DrivingState):
        """
        :param new_state: DrivingState
        Performs a number of internal downstream state updates in response to a drive state update
        """
        old_state = self.driving_state

        self.get_logger().info("------ Drive State Transition: " + str(old_state) + " -> " + str(new_state))
        self.driving_state = new_state

        if new_state == DrivingState.TURNING:
            self.ctl_spin = SpinController(self.state_rover_pose.theta, self.ctl_driver)
        
        if new_state == DrivingState.SUCCESS:
            self.get_logger().debug(f"Entering success drive mode, getting next goal")
            self.reset_goals_and_waypoints()
            self.pub_at_goal.publish(Empty())

        if new_state == DrivingState.PRE_RESET:
            if old_state != DrivingState.RESET_WAIT:
                self.saved_state = old_state

            if self.last_reset is not None and ((self.get_clock().now() - self.last_reset) < Duration(seconds=10)):
                self.get_logger().info(f"Tying to reset within {(self.get_clock().now() - self.last_reset).nanoseconds/1e9}", throttle_duration_sec=1)
                self.on_drive_state_update(DrivingState.RESET_WAIT)

            else:
                for blcmd_id in self.resolver_errors:
                    self.get_logger().info(f"Resetting Resolver {blcmd_id}")
                    if self.auto_mode:
                        msg = BLCMDReset()
                        msg.id = blcmd_id
                        msg.type = BLCMDReset.RESOLVER
                        self.pub_blcmd_reset.publish(msg)
                    else:
                        self.get_logger().info(f"Not in auto mode, could not reset resolver {blcmd_id}")


            self.get_logger().info(f"Reset resolvers")
            self.reset_time = self.get_clock().now()
            self.get_logger().debug(f"Reset time set to {self.reset_time}")

        if new_state == DrivingState.POST_RESET:
            self.get_logger().info(f"Resetting any stall faults")
            for blcmd_id in self.blcmd_errors:
                if self.auto_mode:
                    self.get_logger().info(f"Resetting BLCMD {blcmd_id}")
                    msg = BLCMDReset()
                    msg.id = blcmd_id
                    msg.type = BLCMDReset.BLCMD
                    self.pub_blcmd_reset.publish(msg)
                else:
                    self.get_logger().info(f"Not in auto mode, could not reset BLCMD {blcmd_id}")

            self.get_logger().info(f"last_reset time set to {self.reset_time}")
            self.last_reset = self.get_clock().now()
            self.get_logger().info(f"Reset blcmds")
            self.reset_time = self.get_clock().now()
            self.get_logger().info(f"Reset time set to {self.reset_time}")

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Simple State Update Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def callback_rover_pose(self):
        """
        Stores the latest rover pose message into our State() variable
        """
        try:
            base_link_tf : Transform = self.tf_buffer.lookup_transform("local_map", "base_link", Time()).transform
            self.get_logger().debug("Found transform from local_map to base_link", once=True)
        except:
            self.get_logger().warn("No transform from local_map to base_link", once=True)
        else:
            self.state_rover_pose = Pose2D()
            self.state_rover_pose.x = base_link_tf.translation.x
            self.state_rover_pose.y = base_link_tf.translation.y
            self.state_rover_pose.theta = transform.quat_to_euler(base_link_tf.rotation)[2]

    def callback_steer(self, msg):
        """
        :param msg: WheelPivotData
        """
        self.var_latest_steer = msg.steer
    
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

    def callback_planner_path(self, msg: Path):
        """
        The callback which is called from the path planner subscriber. We need to:
        - Update list of waypoints in way-point path
        - update best effort goal
        Note that when we are near our goal, best effort goal will be set, but way-point path won't be set.
        :param msg: Waypoints message from the path planner
        """
        self.waypoint_path = [(p.pose.position.x, p.pose.position.y) for p in msg.poses]
        self.num_paths_planned += 1

    def callback_blcmd_status(self, msg: BLCMDStatusArray):
        
        self.blcmd_errors = []
        self.resolver_errors = []

        blcmd : BLCMDStatus
        for blcmd in msg.blcmds:
            if blcmd.stall_fault or blcmd.overspeed_fault or blcmd.gate_fault:
                self.blcmd_errors.append(blcmd.id)
            if blcmd.resolver_fault:
                self.resolver_errors.append(blcmd.id)

        if (self.driving_state not in [DrivingState.PRE_RESET, DrivingState.POST_RESET, DrivingState.RESET_WAIT]) and \
                (len(self.resolver_errors) != 0 or len(self.blcmd_errors) != 0):
            self.on_drive_state_update(DrivingState.PRE_RESET)


    def auto_mode_callback(self, msg: Bool):
        self.auto_mode = msg.data


    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 'Util' Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def get_goal_vector(self) -> np.ndarray:
        """
        Use tf2 lookups to translate the goal state variable into the local map frame, and return it as a 1x2 numpy array
        """
        goal_vector = np.array([self.state_current_planning_destination.position.x, self.state_current_planning_destination.position.y, 0.]) 

        try:
            goal_to_local_map = self.tf_buffer.lookup_transform("local_map", self.state_current_planning_destination.header.frame_id, Time()).transform
            goal_vector = transform.transform_points(goal_to_local_map, goal_vector)[:-1]
            return goal_vector.flatten()
        except Exception as e:
            self.get_logger().warn(f"Failed to look up goal in local map: {e}")
            return None

    def near_current_goal(self) -> bool:
        """
        Function determines (if there is a goal) if we are near the goal. If there is no
        planned goal or state_current_planning_destination, we return False, since we 
        must have a goal to be near it. 
        :return: Boolean value of True if we are near goal (and goal exists), False otherwise
        """
        # Get goal distance according to the kind of goal we are driving to
        goal_dist = 0
        if self.driving_state == DrivingState.TO_TARGET:
            goal_dist = self.param_dist_to_targets
        else:
            goal_dist = self.param_dist_to_search_points

        end_of_path = self.waypoint_path[-1] if len(self.waypoint_path) > 0 else None
        goal_vector = self.get_goal_vector()
        current_pos = np.array([self.state_rover_pose.x, self.state_rover_pose.y])

        self.get_logger().debug(f"End of path: {end_of_path}")
        self.get_logger().debug(f"Goal: {goal_vector}")
        self.get_logger().debug(f"Current position: {current_pos}")

        # evaluate criteria bools
        no_plan = self.num_paths_planned == 0
        empty_path = end_of_path is None

        too_far_from_goal = distance(
            current_pos,
            goal_vector
            ) > self.param_max_goal_achieved_dist

        near_end_of_path =  distance(
            current_pos,
            end_of_path
        ) < goal_dist

        # We can't be near the goal if we haven't planned yet
        if no_plan:
            self.get_logger().debug("Haven't planned paths yet. Not near goal")
            return False
        # If we have planned, and the path is still empty, we must have reached the goal
        elif empty_path:
            self.get_logger().debug(f"No end of path, we are done!")
            return True  # path is only empty if we are at the end of it
        elif near_end_of_path:
            self.get_logger().debug(f"Near end of path")
            if too_far_from_goal:
                # We are too far to have reached the goal, even if we are close to the end of our path
                self.get_logger().debug("Too far from goal to achieve path success")
                return False
            else:
                return True
        else:
            # Not near the end of the path
            return False

    def facing_current_goal(self) -> bool:
        """
        Determines whether our current heading is facing the current goal within a desired threshold
        :return: True if facing goal, False otherwise
        """ 
        goal_vector = self.get_goal_vector()
        our_pose_vector = np.array([self.state_rover_pose.x, self.state_rover_pose.y])
        desired_heading_vector = goal_vector - our_pose_vector

        # normalise desired heading
        desired_heading_vector = desired_heading_vector / np.linalg.norm(desired_heading_vector)

        # get unit vector in direction we are facing
        current_heading_vector = np.array([np.cos(self.state_rover_pose.theta), np.sin(self.state_rover_pose.theta)])

        # dot product gives angle between vectors
        angle_between_headings = np.arccos(np.dot(desired_heading_vector, current_heading_vector)) 

        self.get_logger().debug(f"desired_heading: {desired_heading_vector}")
        self.get_logger().debug(f"current_heading: {current_heading_vector}")
        self.get_logger().debug(f"angle between: {angle_between_headings}")

        return angle_between_headings < self.param_goal_facing_threshold 

    def prune_waypoints(self):
        """
        Return a sub-list of the planning waypoints containing only those at least some minimum distance from
        the rover
        """
        if self.waypoint_path is None:
            return []
        return [point for point in self.waypoint_path if distance(
            (self.state_rover_pose.x, self.state_rover_pose.y),
            point
            ) > self.param_waypoint_follow_distance]

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
        self.get_logger().debug(f"driving to {target_waypoint} from {self.state_rover_pose}")

        position_vector = np.array([self.state_rover_pose.x, self.state_rover_pose.y, 0])
        target_vector = np.array([target_waypoint[0], target_waypoint[1], 0])

        desired_orientation = target_vector - position_vector
        desired_orientation /= np.linalg.norm(desired_orientation)

        current_orientation = np.array([np.cos(self.state_rover_pose.theta), np.sin(self.state_rover_pose.theta), 0])

        yaw_diff = yaw_difference(current_orientation, desired_orientation)

        self.get_logger().debug(f"desired: {desired_orientation}, current: {current_orientation}, yaw_diff: {yaw_diff}")

        speed, steer = self.ctl_driver.get_drive_command(yaw_diff, self.var_latest_steer, position_vector, current_orientation)
        return speed, steer

    def control(self):
        """
        Called once every tick by the node's timer. Identifies the next target waypoint
        and calls navigate_to_waypoint, and determines when the rover has arrived
        """
        if self.state_rover_pose is None:
            return
        self.drive_mode_state_transition()
        self.get_logger().debug("Controller in driving state: " + str(self.driving_state))

        drive, steer = 0, 0

        if self.driving_state == DrivingState.SUCCESS:
            self.get_logger().debug("Controller mode: success")

        # -------------------------------------- 0. TURNING ------------------------------
        elif self.driving_state == DrivingState.TURNING:
            self.get_logger().debug("Turning in place")
            current_orientation = np.array([np.cos(self.state_rover_pose.theta), np.sin(self.state_rover_pose.theta), 0.])

            position_vector = np.array([self.state_rover_pose.x, self.state_rover_pose.y])
            drive, steer = self.ctl_spin.turn_in_place(self.var_latest_steer, current_orientation, position_vector=position_vector)

        # -------------------------------------- 1. DRIVING ------------------------------
        elif self.num_paths_planned < 1:
            self.get_logger().debug("Not enough paths planned")

        elif self.driving_state in [DrivingState.TO_WAYPOINT, DrivingState.TO_TARGET, DrivingState.FACE_TARGET]:
            path = self.prune_waypoints()
            if self.driving_state == DrivingState.FACE_TARGET:
                self.get_logger().debug("Facing target")
                drive, steer = self.go_to_target([self.state_current_planning_destination.position.x, self.state_current_planning_destination.position.y])
            elif len(path) > 0:
                self.get_logger().debug("Driving to waypoint")
                drive, steer = self.go_to_target(path[0])
            else:
                self.get_logger().debug("No more waypoints in path.")
        
        # -------------------------------------- 5. RESET ------------------------------
            
        self.send_drive_cmd(drive, steer)


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
        steer = max(-1.0, min(1.0, float(angular_fraction)))
        drive_cmd_msg.radius = float('inf') if steer == 0 else abs(1/steer - (1 if steer > 1 else -1))
        drive_cmd_msg.direction = 0 if steer == 0 else 1 if steer > 0 else -1

        if self.param_do_tank_turn or self.driving_state in [DrivingState.PRE_RESET, DrivingState.POST_RESET]:
            drive_cmd_msg.mode = DriveInput.TANK
        else:
            drive_cmd_msg.mode = DriveInput.PIVOT

        # Print!
        self.get_logger().debug("Driving at speed {:.4f}, steer {:.4f}".format(
            drive_cmd_msg.speed, drive_cmd_msg.steer
        ))

        # publish to public topic
        self.pub_drive_commands.publish(drive_cmd_msg)


def main(args=None):
    rclpy.init(args=args)
    controller = Controller()
    rclpy.spin(controller)
    controller.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
