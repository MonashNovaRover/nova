#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: Controller
TOPICS:
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        autonomous
AUTHOR(S):      Max Tory, Liam Whittle
CREATION:       07/12/2021
EDITED:         23/04/2023
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
from core.msg import DriveInput, AutonomousGoal, PivotWheelData
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
    SUCCESS = 2  # Completed spinning or driving to the current goal


class Controller(Node):
    """
    Controls the movement of the rover between waypoints determined by the path planner.
    Receives updates about the current pose of the rover and the waypoints to
    navigate between via ros topics autonomous/pose and autonomous/goals. Publishes drive
    commands to auto_drive_commands
    """

    def __init__(self):
        super().__init__('autonomous_controller')

        # set debug to not get shown
        self.get_logger().set_level(logging.INFO)

        # Ros params
        self.param_do_tank_turn = self.declare_parameter("do_tank_turn", False).value
        self.param_waypoint_follow_distance = self.declare_parameter("waypoint_follow_distance_m", 0.3).value

        # ~~~~~~~~~~ State ~~~~~~~~
        self.state = DrivingState.SUCCESS
        self.state_rover_pose = Pose2D()
        self.state_waypoint_path = []
        self.state_latest_steer = 0
        self.state_turning_mode = TurningMode.TANK if self.param_do_tank_turn else TurningMode.PIVOT

        self.trigger_spin = False
        self.trigger_to_waypoint = False

        # Controller classes for turning, driving to waypoints, and spinning
        self.ctl_driver : DriveController = DriveController(self.state_turning_mode)
        self.ctl_spin : SpinController = None

        # ------------- ROS Things ----------
        # tf2
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(buffer=self.tf_buffer, node=self, spin_thread=True)

        # Publishers
        self.pub_drive_commands = self.create_publisher(DriveInput, auto_drive_command_topic, 10)
        self.pub_at_goal = self.create_publisher(Empty, "~/goal_achieved", 10)
        self.pub_done_spin = self.create_publisher(Empty, "~/spin_achieved", 10)

        # Subscribers
        self.sub_planned_path = self.create_subscription(Path, auto_waypoints_topic,
                                                                        self.callback_planner_path, 10)
        self.sub_steer = self.create_subscription(PivotWheelData, "/control/pivot_wheel", self.callback_steer, 10)
        self.sub_do_spin = self.create_subscription(Empty, "~/do_spin", self.callback_do_spin, 10)

        self.get_logger().info("Waiting for transform from 'local_map' to 'base_link'...")
        while not self.tf_buffer.can_transform('base_link', 'map', Time()):
            time.sleep(0.1)
        self.get_logger().info("Received Transform!")

        # Timers
        self.timer_control = self.create_timer(0.1, self.control)  # calculate and send drive commands
        self.timer_pose = self.create_timer(0.1, self.callback_rover_pose)  # update the rover's pose from tf2

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ State Transition Helper Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def turn_completed(self) -> bool:
        """
        Determines whether we have completed a turn
        """
        if self.ctl_spin is None:
            return False
        return self.ctl_spin.is_completed()

    def finished_waypoint_path(self) -> bool:
        """
        Returns True if there are no waypoints left in the path, or we are sufficiently
        close to the last waypoint
        """
        if len(self.state_waypoint_path) == 0:
            return True
        
        our_pos = np.array([self.state_rover_pose.x, self.state_rover_pose.y])
        last_waypoint = self.state_waypoint_path[-1]

        return distance(our_pos, last_waypoint) < self.param_waypoint_follow_distance

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ State Transition Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def drive_mode_state_transition(self):
        """
        Update current driving mode based on received triggers and internal state of the state machine
        """
        
        # If we are in SUCCESS state, transitioning to the driving state is triggered by receiving a ros2 message
        if self.state == DrivingState.SUCCESS:
            if self.trigger_spin:
                self.on_state_update(DrivingState.TURNING)
            elif self.trigger_to_waypoint:
                self.on_state_update(DrivingState.TO_WAYPOINT)

        # If we are in TURNING state, we are only done turning when the spin controller says so
        elif self.state == DrivingState.TURNING:
            if self.turn_completed():
                self.on_state_update(DrivingState.SUCCESS)

        # If we are in TO_WAYPOINT state, we are done when we are out of goals to go to
        elif self.state == DrivingState.TO_WAYPOINT:
            if self.finished_waypoint_path():
                self.on_state_update(DrivingState.SUCCESS)

    def on_state_update(self, new_state: DrivingState):
        """
        Set current state to new_state, and perform any necessary state changes
        """
        # Do the state update
        old_state = self.state
        self.get_logger().info(f"------ State Transition: {old_state} -> {new_state}")
        self.get_logger().debug(f"Before transition:\n"
                                f"trigger_spin: {self.trigger_spin}\n"
                                f"trigger_to_waypoint: {self.trigger_to_waypoint}\n"
                                f"Spin controller: {self.ctl_spin}\n"
                                f"Waypoints: {self.state_waypoint_path}\n"
                                )
        self.state = new_state

        # Perform any necessary state changes
        # Entering turning state we initialise a new spin controller
        if self.state == DrivingState.TURNING:
            self.ctl_spin = SpinController(self.state_rover_pose.theta, self.ctl_driver)
            self.trigger_spin = False
        # Entering to waypoint state, reset the trigger
        elif self.state == DrivingState.TO_WAYPOINT:
            self.trigger_to_waypoint = False
        # Entering success, clear state variables and await new trigger
        elif self.state == DrivingState.SUCCESS:
            self.ctl_spin = None
            self.state_waypoint_path = []
            self.state_latest_steer = 0

        self.get_logger().debug(f"After transition:\n"
                                f"trigger_spin: {self.trigger_spin}\n"
                                f"trigger_to_waypoint: {self.trigger_to_waypoint}\n"
                                f"Spin controller: {self.ctl_spin}\n"
                                f"Waypoints: {self.state_waypoint_path}\n"
                                )
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
            self.state_rover_pose.x = base_link_tf.translation.x
            self.state_rover_pose.y = base_link_tf.translation.y
            self.state_rover_pose.theta = transform.quat_to_euler(base_link_tf.rotation)[2]

    def callback_steer(self, msg: PivotWheelData):
        """
        :param msg: WheelPivotData
        """
        self.state_latest_steer = msg.steer

    def callback_planner_path(self, msg: Path):
        """
        The callback which is called from the path planner subscriber. We need to:
        - Update list of waypoints in way-point path
        - update best effort goal
        Note that when we are near our goal, best effort goal will be set, but way-point path won't be set.
        :param msg: Waypoints message from the path planner
        """
        self.state_waypoint_path = [(p.pose.position.x, p.pose.position.y) for p in msg.poses]

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 'Util' Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def completed_path(self) -> bool:
        """
        TODO
        """
        pass

    def prune_waypoints(self):
        """
        Return a sub-list of the planning waypoints containing only those at least some minimum distance from
        the rover
        """
        if self.state_waypoint_path is None:
            return []
        return [point for point in self.state_waypoint_path if distance(
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

        speed, steer = self.ctl_driver.get_drive_command(yaw_diff, self.state_latest_steer, position_vector, current_orientation)
        return speed, steer

    def control(self):
        """
        Called once every tick by the node's timer. Identifies the next target waypoint
        and calls navigate_to_waypoint, and determines when the rover has arrived
        """
        if self.state_rover_pose is None:
            return
        self.drive_mode_state_transition()
        self.get_logger().debug("Controller in driving state: " + str(self.state))

        drive, steer = 0, 0

        if self.state == DrivingState.SUCCESS:
            self.get_logger().debug("Controller mode: success")

        # -------------------------------------- 0. TURNING ------------------------------
        elif self.state == DrivingState.TURNING:
            self.get_logger().debug("Turning in place")
            current_orientation = np.array([np.cos(self.state_rover_pose.theta), np.sin(self.state_rover_pose.theta), 0.])

            position_vector = np.array([self.state_rover_pose.x, self.state_rover_pose.y])
            drive, steer = self.ctl_spin.turn_in_place(self.state_latest_steer, current_orientation, position_vector=position_vector)

        # -------------------------------------- 1. DRIVING ------------------------------
        elif self.state_num_paths_planned < 1:
            self.get_logger().debug("Not enough paths planned")

        elif self.state == DrivingState.TO_WAYPOINT:
            path = self.prune_waypoints()
            self.get_logger().debug("Driving to waypoint")
            drive, steer = self.go_to_target(path[0])
        
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
        drive_cmd_msg.steer = max(-1.0, min(1.0, float(angular_fraction)))

        if self.param_do_tank_turn or self.state in [DrivingState.PRE_RESET, DrivingState.POST_RESET]:
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