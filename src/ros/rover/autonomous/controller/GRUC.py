#!/usr/bin/env python3
__package__ = "autonomous"

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
EDITED:         15/05/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

# ros import
import rclpy
from rclpy.node import Node
from std_srvs.srv import Trigger

# custom message imports
from core.msg import DriveInput, RoverPose, Waypoints, AlvarMarker, AutonomousGoal, Point2D
from controller.ar_tag_manager import ArTagManager
from controller.spin_controller import SpinController

# autonomous imports
from math_utils.controller_math import *
from config.runtime_params import *
from config.ros_config import *
from planning.path_planner import PathPlanner
from controller.turning import YawStarTurner
from controller.drive_controller import DriveController


class Controller(Node):
    """
    Controls the movement of the rover between waypoints determined by the path planner.
    Receives updates about the current pose of the rover and the waypoints to
    navigate between via ros topics autonomous/pose and autonomous/goals. Publishes drive
    commands to auto_drive_commands
    """
    SEARCH = 0
    HONING = 1
    GATE = 2
    SUCCESS = 3

    TO_WAYPOINT = 10
    TURNING = 11

    def __init__(self):
        super().__init__('autonomous_controller_node')
        self.planners = {
            Controller.SEARCH: self.plan_search,
            Controller.HONING: self.get_current_goal,
            Controller.GATE: self.plan_through_gate,
            Controller.SUCCESS: None
        }

        # Controller classes
        self.turner = YawStarTurner()
        self.driver = DriveController(self.turner)
        self.spin_controller = None

        # State
        self.state = State()  # from controller_math
        self.path = []
        self.previous_goals = []
        self.original_goal = None
        self.gate_goal_pose = None
        self.achieved_original_goal = False
        self.search_plan = []
        self.search_array_index = -1
        self.current_best_goal = None
        self.ar_tag_manager = ArTagManager()

        # on init, it should not drive anywhere until update_goals is called
        self.planning_mode = Controller.SUCCESS
        self.driving_mode = Controller.TO_WAYPOINT

        # ------------- ROS Things ----------
        # Publishers
        self.drive_input_publisher = self.create_publisher(DriveInput, auto_drive_command_topic, 10)
        self.planning_destination_publisher = self.create_publisher(Point2D, planning_destination_topic, 10)

        # Subscribers
        self.pose_subscriber = self.create_subscription(RoverPose, rover_pose_topic, self.update_pose, 10)
        self.ar_tag_subscriber = self.create_subscription(AlvarMarker, ar_track_topic, self.update_ar_tag, 10)
        self.update_goal_subscriber = self.create_subscription(AutonomousGoal, auto_goal_topic, self.update_goal, 10)
        self.path_subscriber = self.create_subscription(Waypoints, auto_waypoints_topic, self.update_path_sub, 10)

        self.led_client = self.create_client(Trigger, "/autonomous/led")

        # Timers
        # Controls the rate at which drive commands are sent - sleeps for the necessary time to maintain the rate give
        self.timer = self.create_timer(0.1, self.control)
        self.planning_timer = self.create_timer(1.0, self.plan)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Simple State Update Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

    def update_path_sub(self, msg):
        self.path = [(p.x, p.y) for p in msg.waypoints]
        return self.path

    def update_pose(self, msg):
        """
        Callback for pose topic
        Callback function that updates the current pose of the rover from data in the auto_command_pose_updates topic
        """
        print("got new pose")
        self.state.x = msg.x
        self.state.y = msg.y
        self.state.yaw = msg.yaw
        self.state.velocity = msg.velocity
        self.state.angular_velocity = msg.angular_velocity

    def update_goal(self, msg):
        """
        Callback for autonomous_goal topic
        When this callback is called, it means we have received two different pieces of information:
            - a new goal coordinate to drive to
            - zero, one, or two AR tag ids to travel to:
                - zero means we ignore AR tags (but store them for later), one means we search for the AR tag,
                and two means we are looking for a gate
        """
        # we update the state of AR tag ids so that it can compare AR tags to the ones we care about
        self.ar_tag_manager.ar_tag_goals = [iD for iD in msg.ids]
        self.original_goal = msg.position.x, msg.position.y
        self.planning_mode = Controller.HONING

    def update_ar_tag(self, msg):
        """
        :param msg: AlvarMarker msg type, received from the ar_tag_topic
        """
        # we pass in our current state so the tag manager knows how to transfer the AR tag pose to a global frame
        self.ar_tag_manager.update_tags(msg, self.state)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ 'Util' Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def setup_search(self):
        self.search_plan = interpolate_circle_points(self.original_goal)
        self.driving_mode = Controller.TURNING

    def near_position(self):
        """
        Function determines (if there is a goal) if we are near the goal.
        :return: Boolean value of True if we are near goal (and goal exists), False otherwise
        """
        if self.current_best_goal is None:
            return False
        return distance(np.array(self.state.x, self.state.y), np.array(self.current_best_goal)) < min_waypoint_distance

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Planning Loop Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def get_current_goal(self):
        """
        Calculates the current goal as either the "original" goal, or the average vector of a bunch of AR tags
        """
        # return
        if self.ar_tag_manager.found_current_goal():
            return self.ar_tag_manager.get_average_goal_pose()

        # If we weren't looking for or haven't found ar tags, go to the original goal
        else:
            # If we have found ar tags, go to their average position
            return self.original_goal

    def plan_search(self):
        """
        :return:
        """
        if self.near_position() or self.search_array_index == -1:
            self.search_array_index += 1
        return self.search_plan[self.search_array_index]

    def plan_through_gate(self):
        """
        Calculates a point some distance through the gate. Assumes we are already
        at the gate, and also that we are facing the direction
        """
        assert self.ar_tag_manager.num_tags_found() == 2 and "Tried to path through a gate without two points!"

        gate_id_1, gate_id_2 = self.ar_tag_manager.ar_tag_goals[0], self.ar_tag_manager.ar_tag_goals[1]
        gate_l, gate_r = np.array(self.ar_tag_manager.get_average_tag_pose(gate_id_1),
                                  np.array(self.ar_tag_manager.get_average_tag_pose(gate_id_1)))
        gate_mid = 0.5 * (gate_l + gate_r)

        # negative reciprocal gives perpendicular vector to the vector between the gate
        gate_perpendicular = np.array([(gate_r - gate_l)[1], (gate_l - gate_r)[0]])
        # Current yaw as vector
        orientation_vector = np.array([np.cos(self.state.yaw), np.sin(self.state.yaw)])
        # -1 if we are facing away from perpendicular vector, +1 if we are towards it
        direction = np.sign(np.dot(orientation_vector, gate_perpendicular))

        # Vector from the middle of the two gate poles to our target
        centre_of_gate_to_target = direction * dist_through_gate_m * gate_perpendicular / magnitude(gate_perpendicular)
        goal = gate_mid + centre_of_gate_to_target
        self.gate_goal_pose = goal

    def plan(self):
        if self.planning_mode == Controller.SUCCESS:
            return
        planning_destination = Point2D()
        planning_destination.x, planning_destination.y = self.planners[self.planning_mode]()
        self.planning_destination_publisher.publish(planning_destination)

    # ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ Control Loop Methods ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    def check_if_completed(self):
        # Check if we've achieved our goals :)
        completed = False
        if len(self.ar_tag_ids) == 0:
            # Just check that we're near the original goal
            completed = self.achieved_original_goal

        elif len(self.ar_tag_ids) == 1:
            # Need to check that we've found the ar tag and we are near it
            ar_tag_pose = self.ar_tag_poses[self.ar_tag_ids[0]]
            completed = (ar_tag_pose is not None and self.near_position())

        elif len(self.ar_tag_ids) == 2:
            completed = self.planning_mode = Controller.GATE and \
                                             self.near_position()

        if completed:
            self.completed_routine()

    def completed_routine(self):
        self.planning_mode = Controller.SUCCESS
        trigger = Trigger.Request()
        self.led_client.call_async(trigger)

        if len(self.ar_tag_ids) == 0:
            previous = self.original_goal
        else:
            found_poses = [self.ar_tag_poses[_id] for _id in self.ar_tag_ids]
            previous = average_vector(np.array(found_poses))
        self.previous_goals.append(previous)

    def go_to_target(self, target_waypoint):
        """
        Publishes a single drive command to navigate to the current target waypoint.
        Called every tick by the control method. Turns in place to face towards the waypoint,
        or drives towards it in a straight line. If the rover has just finished turning, a
        single zero drive command is sent before driving begins.
        """
        # calculate target yaw and signed yaw difference using the controller_math module
        print(f"driving to {target_waypoint}")
        position_vector = np.array([self.state.x, self.state.y, 0])
        target_vector = np.array([target_waypoint[0], target_waypoint[1], 0])

        desired_orientation = target_vector - position_vector
        current_orientation = np.array([np.cos(self.state.yaw), np.sin(self.state.yaw), 0])

        yaw_diff = yaw_difference(current_orientation, desired_orientation)

        drive = self.driver.get_drive_command(yaw_diff, position_vector, current_orientation)
        self.__publish(drive['drive'], drive['steer'])

    def control(self):
        """
        Called once every tick by the node's timer. Identifies the next target waypoint
        and calls navigate_to_waypoint, and determines when the rover has arrived
        """
        if self.planning_mode == Controller.SUCCESS or self.path is None:
            return

        found_ar_ids = [_id for _id in self.ar_tag_ids if self.ar_tag_poses[_id] is not None]
        current_orientation = np.array([np.cos(self.state.yaw), np.sin(self.state.yaw), 0])
        if self.driving_mode == Controller.TURNING:
            if self.spin_controller is None:
                self.spin_controller = SpinController(self.state.yaw, self.turner)
            if len(found_ar_ids) == 0 and not self.spin_controller.completed():
                drive = self.spin_controller.turn_in_place(current_orientation)
                self.__publish(drive["drive"], drive["steer"])
            else:
                self.driving_mode = Controller.TO_WAYPOINT

        if self.driving_mode == Controller.TO_WAYPOINT:
            # can only go to waypoints
            # Drive to a waypoint
            while True:
                if len(self.path) == 0:
                    break

                target_waypoint = self.path[0]

                if distance((self.state.x, self.state.y), target_waypoint) > min_waypoint_distance:
                    # we have not yet arrived at the waypoint
                    self.go_to_target(target_waypoint)
                    break

                else:
                    # If distance to the waypoint is lower than the threshold distance, we have arrived
                    self.get_logger().info("Reached way-point: " + str(target_waypoint))
                    self.path = self.path[1:]

        # update Mode
        if len(self.ar_tag_ids) != 0 \
                and len(found_ar_ids) == 0 \
                and self.achieved_original_goal:
            if not self.planning_mode == Controller.SEARCH:
                self.current_best_goal = None
                self.setup_search()

            self.planning_mode = Controller.SEARCH

        else:
            if self.planning_mode != Controller.HONING:
                self.current_best_goal = None
                self.planning_mode = Controller.HONING

        if self.planning_mode == Controller.HONING \
                and len(found_ar_ids) == 2 \
                and self.near_position():
            # We were in honing mode, we've found the gate, and we're at the gate
            self.current_best_goal = None
            self.planning_mode = Controller.GATE
        self.check_if_completed()

        # Updating state
        if self.near_position():
            self.achieved_original_goal = True

    def __publish(self, drive_fraction, angular_fraction):
        """
        Publishes drive commands to the auto_drive_commands topic
        :param: drive_fraction: [-1:1]
        :param: angular_fraction: [-1:1]
        """

        # construct message to publish
        drive_cmd_msg = DriveInput()

        print("driving at: " + str(drive_fraction) + " | " + " with speed: " + str(angular_fraction))
        drive_cmd_msg.speed = float(drive_fraction)

        drive_cmd_msg.steer = float(angular_fraction)

        # publish to public topic
        self.drive_input_publisher.publish(drive_cmd_msg)


def handle_path_status(self, status):
    """
    Handles logging and adjusting of parameters according to the
    status returned by our c++ A* method.
    """
    if status & PathPlanner.A_STAR_START_OBSTACLE: self.get_logger().warn("started in obstacle")
    if status & PathPlanner.A_STAR_DEST_OBSTACLE: self.get_logger().warn("dest in obstacle")
    if status & PathPlanner.A_STAR_NO_PATH: self.get_logger().warn("couldn't find a path initially")
    if status & PathPlanner.A_STAR_CRITICAL_NO_PATH:
        self.get_logger().error("COULDN'T FIND PATH - NEAR OBSTACLE")
        self.padding_dist_m -= 0.1
        if self.padding_dist_m < 0.4:
            self.get_logger().error("Ah HECK")
            return
    if status == PathPlanner.A_STAR_SUCCESS: self.get_logger().info("A* found safe path")


def main(args=None):
    rclpy.init(args=args)
    controller = Controller()
    rclpy.spin(controller)
    controller.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
