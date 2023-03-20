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
from core.msg import DriveInput, AutonomousGoal, PivotWheelData
from nav_msgs.msg import Path
from autonomous.controller.spin_controller import SpinController

# autonomous imports
from autonomous.math_utils.controller_math import *
import autonomous.math_utils.transform as transform
from autonomous.config.runtime_params import *
from autonomous.config.ros_config import *
from autonomous.controller.goal_manager import GoalManager
from autonomous.controller.drive_controller import DriveController, TurningMode

# misc
from enum import Enum
import logging


class DrivingState(Enum):
    TURNING = 0  # doing a 360-degree turn on the spot
    TO_WAYPOINT = 1  # driving to the next waypoint in a path
    SUCCESS = 2  # Completed driving to the current goal


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
        self.get_logger().set_level(logging.DEBUG)

        # Ros params
        self.param_do_tank_turn = self.declare_parameter("do_tank_turn", False).value

        # ~~~~~~~~~~ State ~~~~~~~~
        self.state_rover_pose = Pose2D()
        self.waypoint_path = []
        self.driving_state = DrivingState.SUCCESS
        self.var_latest_steer = 0
        self.turning_mode = TurningMode.TANK if self.param_do_tank_turn else TurningMode.PIVOT
        self.num_paths_planned = 0

        # Controller classes for turning, driving to waypoints, and spinning
        self.ctl_driver = DriveController(self.turning_mode)
        self.ctl_spin = SpinController(0, self.ctl_driver)

        # ------------- ROS Things ----------
        # tf2
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(buffer=self.tf_buffer, node=self, spin_thread=True)

        # Publishers
        # 'DriveInput' message is used to make the wheels move!
        self.pub_drive_commands = self.create_publisher(DriveInput, auto_drive_command_topic, 10)
        # Planned destination -> we wish to go here, which is the next step on our path to the target

        # Subscribers
        self.sub_planned_path_to_destination = self.create_subscription(Path, auto_waypoints_topic,
                                                                        self.callback_planner_path, 10)
        self.sub_steer = self.create_subscription(PivotWheelData, "/control/pivot_wheel", self.callback_steer, 10)

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
        self.on_drive_state_update(DrivingState.SUCCESS)

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
            if current_goal.type == AutonomousGoal.GOAL_TYPE_SPIN and not self.ctl_spin.is_completed():
                self.on_drive_state_update(DrivingState.TURNING)
            elif not self.near_current_goal():
                self.on_drive_state_update(DrivingState.TO_WAYPOINT)
        elif self.driving_state == DrivingState.TO_WAYPOINT:
            if current_goal.type == AutonomousGoal.GOAL_TYPE_SPIN:
                # start spinning
                self.on_drive_state_update(DrivingState.TURNING)
            elif self.near_current_goal():
                # Stop driving to goals when we get close to them
                self.on_drive_state_update(DrivingState.SUCCESS)
        elif self.driving_state == DrivingState.TURNING:
            if current_goal.type != AutonomousGoal.GOAL_TYPE_SPIN:
                # We were turning, but now we're done
                self.on_drive_state_update(DrivingState.TO_WAYPOINT)
            elif self.ctl_spin.is_completed():
                # We have just finished a spin
                self.on_drive_state_update(DrivingState.SUCCESS)

    def on_drive_state_update(self, new_state: DrivingState):
        """
        :param new_state: DrivingState
        Performs a number of internal downstream state updates in response to a drive state update
        """
        print(new_state)
        old_state = self.driving_state

        self.get_logger().info("------ Drive State Transition: " + str(old_state) + " -> " + str(new_state))
        print(new_state)

        self.driving_state = new_state
        print(new_state)

        if new_state == DrivingState.TURNING:
            self.ctl_spin = SpinController(self.state_rover_pose.yaw, self.ctl_driver)
            print(new_state)
        
        if new_state == DrivingState.SUCCESS:
            print(new_state)
            self.get_logger().debug(f"Entering success drive mode, getting next goal")
            self.goal_manager.at_goal()

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Simple State Update Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

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

    def callback_steer(self, msg):
        """
        :param msg: WheelPivotData
        """
        self.var_latest_steer = msg.steer

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 'Util' Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def near_current_goal(self) -> bool:
        """
        Function determines (if there is a goal) if we are near the goal. If there is no
        planned goal or state_current_planning_destination, we return False, since we 
        must have a goal to be near it. 
        :return: Boolean value of True if we are near goal (and goal exists), False otherwise
        """

        # look for a best effort goal, else compare to an original goal
        if self.num_paths_planned == 0:
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
        if self.waypoint_path is None:
            return []
        return [point for point in self.waypoint_path if distance(
            (self.state_rover_pose.x, self.state_rover_pose.y),
            point
            ) > min_waypoint_distance]

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Planning Loop Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
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
        planning_destination.pose.position.x = self.state_current_planning_destination.position.x
        planning_destination.pose.position.y = self.state_current_planning_destination.position.y

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
        self.state_current_planning_destination = self.goal_manager.get_current_goal([self.state_rover_pose.x, self.state_rover_pose.y])
        self.drive_mode_state_transition()
        self.get_logger().debug("Controller in driving state: " + str(self.driving_state), throttle_duration_sec=1)

        if self.driving_state == DrivingState.SUCCESS:
            self.get_logger().debug("Controller mode: success", throttle_duration_sec=1)
            self.send_drive_cmd(0, 0)
            return
        if self.num_paths_planned < 1:
            self.get_logger().debug("Not enough paths planned", throttle_duration_sec=1)
            self.send_drive_cmd(0, 0)
            return

        current_orientation = np.array([np.cos(self.state_rover_pose.yaw), np.sin(self.state_rover_pose.yaw), 0.])

        # -------------------------------------- 0. TURNING ------------------------------
        if self.driving_state == DrivingState.TURNING:
            position_vector = np.array([self.state_rover_pose.x, self.state_rover_pose.y])
            drive, steer = self.ctl_spin.turn_in_place(self.var_latest_steer, current_orientation, position_vector=position_vector)
            self.send_drive_cmd(drive, steer)
            return

        # -------------------------------------- 1. DRIVING ------------------------------
        path = self.prune_waypoints()
        if self.driving_state == DrivingState.TO_WAYPOINT and len(path) > 0:
            # can only go to waypoints
            # Drive to a waypoint
            self.go_to_target(path[0])
        elif len(path) == 0:
            self.get_logger().debug("No more waypoints in path.", throttle_duration_sec=1)
            self.send_drive_cmd(0, 0)

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

        if self.param_do_tank_turn:
            drive_cmd_msg.mode = DriveInput.TANK
        else:
            drive_cmd_msg.mode = DriveInput.PIVOT

        # Print!
        self.get_logger().debug("Driving at speed {:.4f}, steer {:.4f}".format(
            drive_cmd_msg.speed, drive_cmd_msg.steer
        ), throttle_duration_sec=1)

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
