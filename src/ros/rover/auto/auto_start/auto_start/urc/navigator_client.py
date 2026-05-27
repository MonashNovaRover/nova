#!/usr/bin/env python3
from rclpy.node import Node
from rclpy.action import ActionClient
from action_msgs.msg import GoalStatus
from nav2_msgs.action import NavigateThroughPoses
from nova_interfaces.action import URCThroughPoses
from nova_interfaces.srv import CartographerCommand
from os.path import expanduser

class NavigatorClient():
    def __init__(self, node:Node):
        self.node=node
        self.goal_handle=None
        self.status=None
        self.started=False

        # Create action client for /urc_through_poses
        self.urc_navigator_client = ActionClient(self.node, URCThroughPoses, '/urc_through_poses')
        self.node.get_logger().info('Waiting for /urc_through_poses...')
        self.urc_started = self.urc_navigator_client.wait_for_server(timeout_sec=10.0)
        if not self.urc_started:
            self.node.get_logger().error('Failed to find service /urc_through_poses! Exiting.')
            return
        self.node.get_logger().info('Successfully found service /urc_through_poses.')

        # Create action client for /navigate_through_poses
        self.nav2_navigator_client = ActionClient(self.node, NavigateThroughPoses, '/navigate_through_poses')
        self.node.get_logger().info('Waiting for /navigate_through_poses...')
        self.nav2_started = self.nav2_navigator_client.wait_for_server(timeout_sec=10.0)
        if not self.nav2_started:
            self.node.get_logger().error('Failed to find service /navigate_through_poses! Exiting.')
            return
        self.node.get_logger().info('Successfully found service /navigate_through_poses.')

        self.started = self.urc_started and self.nav2_started

    def start(self, type, poses, search_radius):
        self.status = None
        self.goal_handle = None

        match type:

            case CartographerCommand.Request.GNSS:
                self.go_to_gps(poses)

            case CartographerCommand.Request.AR:
                self.go_to_ar_tag(poses, search_radius)

            case CartographerCommand.Request.OBJECT:
                self.go_to_object(poses, search_radius)

    def go_to_gps(self, poses):
        goal_action = NavigateThroughPoses.Goal()
        goal_action.poses = poses
        goal_action.behavior_tree = f"{expanduser("~")}/nova/src/ros/rover/auto/nova_behavior_tree/behavior_tree/shared/nav_through_poses_remove_in_collision_goals.xml"
        self.node.get_logger().info('Sending waypoints to /navigate_through_poses...')
        send_future = self.nav2_navigator_client.send_goal_async(goal_action)
        send_future.add_done_callback(self.response)

    def go_to_ar_tag(self, poses, search_radius):
        goal_action = URCThroughPoses.Goal()
        goal_action.poses = poses
        goal_action.behavior_tree = f"{expanduser("~")}/nova/src/ros/rover/auto/nova_behavior_tree/behavior_tree/urc/urc_through_poses_aruco.xml"
        goal_action.search_radius = search_radius
        self.node.get_logger().info('Sending waypoints to /urc_through_poses...')
        send_future = self.urc_navigator_client.send_goal_async(goal_action)
        send_future.add_done_callback(self.response)

    def go_to_object(self, poses, search_radius):
        goal_action = URCThroughPoses.Goal()
        goal_action.poses = poses
        goal_action.behavior_tree = f"{expanduser("~")}/nova/src/ros/rover/auto/nova_behavior_tree/behavior_tree/urc/urc_through_poses_object.xml"
        goal_action.search_radius = search_radius
        self.node.get_logger().info('Sending waypoints to /urc_through_poses...')
        send_future = self.urc_navigator_client.send_goal_async(goal_action)
        send_future.add_done_callback(self.response)

    def response(self, future):
        goal_handle = future.result()
        if not goal_handle.accepted:
            self.node.get_logger().error('Goal rejected!.')
            return

        self.node.get_logger().info('Goal accepted.')
        self.goal_handle = goal_handle

        # Set up asynchronous result callback
        result_future = goal_handle.get_result_async()
        result_future.add_done_callback(self.result)

    def result(self, future):
        result = future.result()
        self.status = result.status
        self.goal_handle = None

    def cancel_current_goal(self):
        if not self.goal_handle:
            self.node.get_logger().warn('No active goal to cancel.')
            return False

        self.node.get_logger().info('Cancelling active navigation goal...')
        cancel_future = self.goal_handle.cancel_goal_async()
        cancel_future.add_done_callback(self.cancel_response)
        return True

    def cancel_response(self, future):
        cancel_result = future.result()
        if len(cancel_result.goals_canceling) == 0:
            self.node.get_logger().warn('Navigation cancel request was rejected.')
            return

        self.node.get_logger().info('Navigation cancel request accepted.')
        self.status = GoalStatus.STATUS_CANCELED
        self.goal_handle = None

    def finished(self):
        return self.status is not None