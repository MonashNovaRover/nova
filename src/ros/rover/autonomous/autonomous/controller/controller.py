#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
Determines the drive commands sent to the rover
based on the current pose and goal.
Has three states:
    SUCCESS: The rover has reached its goal and is
        awaiting the next
    TURNING: The rover is turning on the spot
    TO_WAYPOINT: The rover is driving to the next
        waypoint in the path returned by the path
        planner
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: autonomous_controller
TOPICS:
    - /autonomous_controller/goal_achieved  [pub]
    - /autonomous_controller/spin_achieved  [pub]
    - /autonomous/drive_inputs              [pub]
    - /autonomous_controller/do_spin        [sub]
    - /control/pivot_wheel                  [sub]
    - /autonomous/waypoints                 [sub]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:        autonomous
AUTHOR(S):      Max Tory, Liam Whittle
CREATION:       07/12/2021
EDITED:         27/04/2023
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# ros import
import rclpy
from rclpy.node import Node
from rclpy.time import Time
from rclpy.duration import Duration
from rclpy.task import Future
from rclpy.qos import QoSReliabilityPolicy, QoSProfile
from tf2_ros import Buffer, TransformListener

# message imports
from core.msg import DriveInput, PivotWheelData
from geometry_msgs.msg import Transform, Pose2D
from nav_msgs.msg import Path
from std_msgs.msg import Empty, Bool, Float64

# autonomous imports
from autonomous.math_utils.controller_math import distance, yaw_difference, get_target_radius, wheel_angle_error
from autonomous.math_utils import transform 
from autonomous.config.ros_config import auto_drive_command_topic, auto_waypoints_topic
from autonomous.controller.drive_controller import DriveController, TurningMode

# misc
from enum import Enum
from typing import List
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
    commands 
    State transition diagram at: tinyurl.com/m2uaeycx 
    """

    def __init__(self):
        super().__init__('autonomous_controller')

        # set debug to not get shown
        self.get_logger().set_level(logging.INFO)

        # Ros params
        self.param_do_tank_turn = self.declare_parameter("do_tank_turn", False).value
        self.param_waypoint_follow_distance = self.declare_parameter("waypoint_follow_distance_m", 0.3).value
        self.param_max_speed = self.declare_parameter("max_speed", 0.35).value
        self.param_near_obstacle_speed = self.declare_parameter("near_obstacle_speed", 0.25).value
        self.param_very_near_obstacle_speed = self.declare_parameter("very_near_obstacle_speed", 0.1).value
        self.param_max_wheel_angle_err = self.declare_parameter("max_wheel_err_rads", np.pi / 4).value
        self.param_near_goal_speed = self.declare_parameter("near_goal_speed", 0.25).value
        self.param_near_goal_dist = self.declare_parameter("near_goal_dist", 10).value
        self.param_big_turn_speed = self.declare_parameter("big_turn_speed", 0.15).value
        self.param_big_turn_radius = self.declare_parameter("big_turn_radius_m", 0.2).value
        self.param_min_spin_complete_yaw_diff = self.declare_parameter("min_spin_complete_yaw_diff", np.pi / 8).value
        self.param_max_spin_complete_yaw_diff = self.declare_parameter("max_spin_complete_yaw_diff", np.pi / 4).value

        # ~~~~~~~~~~ State ~~~~~~~~
        self.state = None
        self.state_rover_pose = Pose2D()
        self.state_waypoint_path = []
        self.state_latest_radius = 0
        self.state_turning_mode = TurningMode.TANK if self.param_do_tank_turn else TurningMode.PIVOT
        self.state_spin_start_heading = None
        self.state_near_goal = False
        self.state_near_obstacle = False
        self.state_very_near_obstacle = False

        self.trigger_spin = False
        self.trigger_to_waypoint = False
        self.trigger_success = False

        # Controller classes for turning, driving to waypoints, and spinning
        self.ctl_driver : DriveController = DriveController(self.state_turning_mode)

        # ------------- ROS Things ----------
        # tf2
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(buffer=self.tf_buffer, node=self, spin_thread=True)

        # Drive commands publisher QoS
        deadline = Duration(nanoseconds=2e8)        
        self.qos = QoSProfile(reliability=QoSReliabilityPolicy.BEST_EFFORT, depth=1, deadline=deadline)

        # Publishers
        self.pub_drive_commands = self.create_publisher(DriveInput, auto_drive_command_topic, self.qos)
        self.pub_at_goal = self.create_publisher(Empty, "~/goal_achieved", 10)
        self.pub_done_spin = self.create_publisher(Empty, "~/spin_achieved", 10)

        # Subscribers
        self.sub_planned_path = self.create_subscription(Path, auto_waypoints_topic, self.callback_planner_path, 10)
        self.sub_radius = self.create_subscription(PivotWheelData, "/control/pivot_wheel", self.callback_radius, 10)
        self.sub_do_spin = self.create_subscription(Empty, "~/do_spin", self.callback_do_spin, 10)
        self.sub_success = self.create_subscription(Empty, "/autonomous_controller/success_trigger", self.callback_success, 10)
        self.sub_goal_dist = self.create_subscription(Float64, "/autonomous/goal_dist", self.callback_goal_dist, 10)
        self.sub_near_obstacle = self.create_subscription(Bool, "/autonomous/near_obstacle", self.callback_near_obstacle, 10)
        self.sub_very_near_obstacle = self.create_subscription(Bool, "/autonomous/very_near_obstacle", self.callback_very_near_obstacle, 10)

        self.get_logger().info("Waiting for transform from 'local_map' to 'base_link'...")
        self.transform_future : Future = self.tf_buffer.wait_for_transform_async('base_link', 'local_map', Time())
        self.transform_future.add_done_callback(self.callback_set_up_timers)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ State Transition Helper Functions ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def turn_completed(self) -> bool:
        """
        Determines whether we have completed a turn
        """
        heading_vec = np.array([np.cos(self.state_rover_pose.theta), np.sin(self.state_rover_pose.theta), 0.])
        self.get_logger().debug(f"Target heading: {self.state_spin_start_heading}, current heading: {heading_vec}")
        yaw_diff = yaw_difference(heading_vec, self.state_spin_start_heading)
        return self.param_min_spin_complete_yaw_diff < yaw_diff < self.param_max_spin_complete_yaw_diff

    def finished_waypoint_path(self) -> bool:
        """
        Returns True if there are no waypoints left in the pruned path
        """
        return self.state_waypoint_path is not None and len(self.state_waypoint_path) == 0

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ State Transition Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def drive_mode_state_transition(self):
        """
        Update current driving mode based on received triggers and internal state of the state machine.
        State transition diagram at: tinyurl.com/m2uaeycx 
        """
        
        # If we are in SUCCESS state, transitioning to the driving state is triggered by receiving a ros2 message
        if self.state == DrivingState.SUCCESS:
            if self.trigger_spin:
                self.on_state_update(DrivingState.TURNING)
            elif self.trigger_to_waypoint:
                self.on_state_update(DrivingState.TO_WAYPOINT)

        # If we are in TURNING state, we are only done turning when the spin controller says so, or when we receive a
        # trigger from the goal_selector state machine
        elif self.state == DrivingState.TURNING:
            if self.turn_completed():
                self.on_state_update(DrivingState.SUCCESS)
            elif self.trigger_success:
                self.on_state_update(DrivingState.SUCCESS)

        # If we are in TO_WAYPOINT state, we are done when we are out of goals to go to, or when we receive a message
        # From the goal_selector state machine telling us to begin a spin or enter success mode
        elif self.state == DrivingState.TO_WAYPOINT:
            if self.trigger_spin:
                self.on_state_update(DrivingState.TURNING)
            elif self.trigger_success:
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
                                f"Waypoints: {self.state_waypoint_path}\n"
                                f"Spin start yaw: {self.state_spin_start_heading}\n"
                                )
        self.state = new_state

        # Perform any necessary state changes
        # Entering turning state we initialise a new spin controller
        if self.state == DrivingState.TURNING:
            self.state_spin_start_heading = np.array([np.cos(self.state_rover_pose.theta), np.sin(self.state_rover_pose.theta), 0])
        # Entering to waypoint state, reset the trigger
        elif self.state == DrivingState.SUCCESS:
            self.state_spin_start_heading = None
            self.state_waypoint_path = []
            self.state_latest_radius = 0

            if old_state == DrivingState.TURNING and not self.trigger_success:
                self.pub_done_spin.publish(Empty())

        self.trigger_spin = False
        self.trigger_success = False
        self.trigger_to_waypoint = False

        self.get_logger().debug(f"After transition:\n"
                                f"trigger_spin: {self.trigger_spin}\n"
                                f"trigger_to_waypoint: {self.trigger_to_waypoint}\n"
                                f"Waypoints: {self.state_waypoint_path}\n"
                                f"Spin start yaw: {self.state_spin_start_heading}\n"
                                )

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ ROS callbacks ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def callback_set_up_timers(self, future: Future):
        """
        Called once we have received the transform from local_map to base_link
        """
        try:
            future.result()
        except Exception as e:
            self.get_logger().error(f"Failed to get transform: {e}")
            self.destroy_node()
        else:
            self.get_logger().info("Received Transform!")

            # Timers
            self.timer_control = self.create_timer(0.1, self.control)  # calculate and send drive commands
            self.timer_pose = self.create_timer(0.1, self.callback_rover_pose)  # update the rover's pose from tf2
            self.on_state_update(DrivingState.SUCCESS)

    def callback_rover_pose(self):
        """
        Stores the latest rover pose into our Pose2D variable
        """
        try:
            base_link_tf : Transform = self.tf_buffer.lookup_transform("map", "base_link", Time()).transform
            self.get_logger().debug("Found transform from local_map to base_link", once=True)
        except Exception as e:
            self.get_logger().warn(f"No transform from local_map to base_link: {e}", throttle_duration_sec=1)
        else:
            self.get_logger().debug(f"base_link_tf: {base_link_tf}", throttle_duration_sec=1)
            self.state_rover_pose.x = base_link_tf.translation.x
            self.state_rover_pose.y = base_link_tf.translation.y
            self.state_rover_pose.theta = transform.quat_to_euler(base_link_tf.rotation)[2]

    def callback_radius(self, msg: PivotWheelData):
        """
        :param msg: WheelPivotData
        """
        radius, direction = msg.radius, msg.direction
        if direction == 0:
            self.state_latest_radius = float('inf')
        else:
            signed_radius = radius * direction
            self.state_latest_radius = signed_radius

    def callback_planner_path(self, msg: Path):
        """
        The callback which is called from the path planner subscriber. We need to:
        - Update list of waypoints in way-point path with a pruned set of waypoints from the path planner
        - Set the to_waypoint trigger to True
        :param msg: Waypoints message from the path planner
        """
        try:
            local_map_to_map : Transform = self.tf_buffer.lookup_transform("map", "local_map", Time.from_msg(msg.header.stamp), Duration(nanoseconds=1e8)).transform
            self.get_logger().debug(f"transforming path by transform: {local_map_to_map}")
        except Exception as e:
            self.get_logger().warn(f"Couldn't get transform from local_map to map: {e}", throttle_duration_sec=1)
            return
        transformed_path = [transform.transform_pose(p.pose, local_map_to_map) for p in msg.poses]
        points = [(p.position.x, p.position.y) for p in transformed_path]
        self.state_waypoint_path = self.prune_waypoints(points)

        if len(self.state_waypoint_path) > 0:
            self.trigger_to_waypoint = True 

    def callback_success(self, msg: Empty):
        """
        Callback for the success trigger subscriber. Sets the success trigger to True
        :param msg: Empty message
        """
        self.trigger_success = True

    def callback_do_spin(self, msg: Empty):
        """
        Callback for the spin trigger subscriber. Sets the spin trigger to True
        :param msg: Empty message
        """
        self.trigger_spin = True

    def callback_goal_dist(self, msg: Float64):
        """
        Callback for the near goal subscriber. Sets the near goal trigger to True
        :param msg: Bool message
        """
        self.state_near_goal = msg.data < self.param_near_goal_dist

    def callback_near_obstacle(self, msg: Bool):
        """
        Callback for the near goal subscriber. Sets the near goal trigger to True
        :param msg: Bool message
        """
        self.state_near_obstacle = msg.data

    def callback_very_near_obstacle(self, msg: Bool):
        """
        Callback for the near goal subscriber. Sets the near goal trigger to True
        :param msg: Bool message
        """
        self.state_very_near_obstacle = msg.data


    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 'Util' Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def prune_waypoints(self, points: List[tuple]) -> bool:
        """
        Return a sub-list of the planning waypoints containing only those at least some minimum distance from
        the rover
        """
        if points is None:
            return []
        current_position = (self.state_rover_pose.x, self.state_rover_pose.y)
        return [point for point in points if 
                distance(current_position, point) 
                    > self.param_waypoint_follow_distance]

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Control Loop Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def get_yaw_difference(self, target_waypoint: tuple) -> float:
        # Calculate the necessary relative change in Yaw to face the target
        position_vector = np.array([self.state_rover_pose.x, self.state_rover_pose.y, 0])
        target_vector = np.array([target_waypoint[0], target_waypoint[1], 0])

        # Unit vector from us to the target
        desired_orientation = target_vector - position_vector
        desired_orientation /= np.linalg.norm(desired_orientation)

        # Current orientation as a unit vector
        current_orientation = np.array([np.cos(self.state_rover_pose.theta), np.sin(self.state_rover_pose.theta), 0])

        self.get_logger().debug(f"desired: {desired_orientation}, current: {current_orientation}", throttle_duration_sec=1)

        return yaw_difference(current_orientation, desired_orientation)

    def get_drive_command(self, yaw_diff):
        # Turn radius for this yaw difference
        signed_radius = get_target_radius(yaw_diff)

        # Turn in the opposite direction of our yaw difference as we consider turns to the left to be negative
        direction = int(-np.sign(yaw_diff))

        # Get a factor to scale our speed by based on our current wheel pivot error
        wheel_angle_rads = wheel_angle_error(signed_radius, self.state_latest_radius)
        scaled_angle_error = min(wheel_angle_rads / self.param_max_wheel_angle_err, 1)
        wheel_error_speed_factor = 1 - scaled_angle_error**2

        # Scale speed by how sharply we are turning
        # Clamp radius between 0.5 and 2 m
        radius = abs(signed_radius)

        if radius < self.param_big_turn_radius:
            speed = self.param_big_turn_speed
        elif self.state_very_near_obstacle:
            speed = self.param_very_near_obstacle_speed
        elif self.state_near_obstacle:
            speed = self.param_near_obstacle_speed
        elif self.state_near_goal:
            speed = self.param_near_goal_speed
        else:
            speed = self.param_max_speed
        
        speed *= wheel_error_speed_factor

        self.get_logger().debug(f"Turn radius: {radius}")
        self.get_logger().debug(f"Current radius: {self.state_latest_radius}")
        self.get_logger().debug(f"Direction: {direction}")
        self.get_logger().debug(f"wheel angle error: {wheel_angle_rads}")
        self.get_logger().debug(f"wheel error speed factor: {wheel_error_speed_factor}")
        self.get_logger().debug(f"speed: {speed}")
        return speed, radius, direction

    def control(self):
        """
        Called once every tick by the node's timer. Identifies the next target waypoint
        and calls navigate_to_waypoint, and determines when the rover has arrived
        """
        if self.state_rover_pose is None:
            return
        self.state_waypoint_path = self.prune_waypoints(self.state_waypoint_path)
        self.drive_mode_state_transition()
        self.get_logger().debug("Controller in driving state: " + str(self.state), throttle_duration_sec=1)

        speed, radius, direction = 0, float('inf'), 0

        if self.state == DrivingState.SUCCESS:
            self.get_logger().debug("Controller mode: success", throttle_duration_sec=1)

        # -------------------------------------- 0. TURNING ------------------------------
        elif self.state == DrivingState.TURNING:
            self.get_logger().debug("Turning in place", throttle_duration_sec=1)
                
            # Get drive commands for a turn 90 degrees to the left
            speed, radius, direction = self.get_drive_command(np.pi)

        # -------------------------------------- 1. DRIVING ------------------------------
        elif self.state == DrivingState.TO_WAYPOINT:
            if self.finished_waypoint_path():
                self.pub_at_goal.publish(Empty())
            elif self.state_waypoint_path is None:
                self.get_logger().error("No waypoints to drive to - This should be detected in state transition!")
                return
            else:
                self.get_logger().debug("Driving to waypoint", throttle_duration_sec=1)
                yaw_diff = self.get_yaw_difference(self.state_waypoint_path[0])
                speed, radius, direction = self.get_drive_command(yaw_diff)
            
        self.send_drive_cmd(speed, radius, direction)

    def send_drive_cmd(self, speed: float, radius: float, direction: int):
        """
        Publishes drive commands to the auto_drive_commands topic
        :param: drive_fraction: [-1:1]
        :param: angular_fraction: [-1:1]
        """
        # construct message to publish
        drive_cmd_msg = DriveInput()
        # Values are validated to stay within -1:1
        drive_cmd_msg.speed = max(-1.0, min(1.0, float(speed)))
        drive_cmd_msg.radius = radius
        drive_cmd_msg.direction = direction

        if self.param_do_tank_turn:
            drive_cmd_msg.mode = DriveInput.TANK
        else:
            drive_cmd_msg.mode = DriveInput.PIVOT

        # Print!
        self.get_logger().debug(
                f"Driving at speed {drive_cmd_msg.speed:.4f}, radius {drive_cmd_msg.radius:.4f}, direction {drive_cmd_msg.direction}", throttle_duration_sec=1
            )

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
