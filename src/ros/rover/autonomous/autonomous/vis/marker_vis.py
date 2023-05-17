#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Listed to AutonomousGoalArray messages 
    and AlvarMarkers and publish Markers showing
    their locations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: path_planner_node
TOPICS:
  - subscriber: /template/subscriber [RoverPose]
  - publisher: /template/publisher [String]
SERVICES:
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:
AUTHOR(S):	Liam Whittle
CREATION:	08/03/2022
EDITED:		08/03/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - change all the template artefacts
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.duration import Duration

from core.msg import AutonomousGoal, AutonomousGoalArray, AlvarMarkers, AlvarMarker
from visualization_msgs.msg import MarkerArray, Marker
from geometry_msgs.msg import PoseStamped

from autonomous.config.ros_config import planning_destination_topic

COLORS = {
    "tag": (0.9, 0.9, 0.9),
    "intermediate": (0.9, 0.45, 0.0),
    "final": (0.0, 0.9, 0.0),
    "planning_goal": (0.0, 0.0, 0.9),
}

class TemplateNode(Node):

    def __init__(self):
        super().__init__("TemplateNode")
        self.sub_goals = self.create_subscription(AutonomousGoalArray, "/autonomous/goal", self.cb_auto_goals, 10)
        self.sub_tags = self.create_subscription(AlvarMarkers, "/ar_tracker/tags", self.cb_ar_tags, 10)
        self.sub_planning_goals = self.create_subscription(PoseStamped, planning_destination_topic, self.cb_planning_goals, 10)

        self.last_goals : AutonomousGoalArray = None
        self.last_tags : AlvarMarkers = None
        self.last_planning_goal : PoseStamped = None

        self.publisher = self.create_publisher(MarkerArray, "/visualisation/markers", 10)

        timer_period = 0.1  # run the timer 10 times per second
        self.create_timer(timer_period, self.timer_callback)

    def cb_auto_goals(self, msg):
        self.last_goals = msg

    def cb_planning_goals(self, msg):
        self.last_planning_goal = msg

    def cb_ar_tags(self, msg):
        self.last_tags = msg

    def get_marker(self, point, c: tuple, index: int, namespace: str) -> None:
        """
        :params: c is color tuple (r,g,b) between 0 and 1
        """
        msg = Marker()
        msg.pose.position.x = point[0]
        msg.pose.position.y = point[1]
        msg.pose.position.z = point[2]
        msg.pose.orientation.w = 1.0
        msg.type = Marker.CYLINDER
        msg.scale.x = .15
        msg.scale.y = .15
        msg.scale.z = 1.0
        msg.color.r = c[0]
        msg.color.g = c[1]
        msg.color.b = c[2]
        msg.color.a = 1.
        msg.lifetime = Duration(seconds=0.2).to_msg()
        # Namespace - raw messages can be separated from confirmed cubes
        msg.ns = namespace
        msg.id = index
        # Survive for half a second
        return msg

    def pub_last_goals(self):
        """
        Publishes the last goals
        """
        msg = MarkerArray()
        goal : AutonomousGoal
        for i, goal in enumerate(self.last_goals.goals):
            color = COLORS["intermediate"] if i < len(self.last_goals.goals) - 1 else COLORS["final"]
            marker : Marker = self.get_marker((goal.position.x, goal.position.y, 0.), color, i, "goal")
            marker.header.frame_id = self.last_goals.header.frame_id
            msg.markers.append(marker)
        self.publisher.publish(msg)

    def pub_last_tags(self):
        """
        Publishes the last goals
        """
        msg = MarkerArray()
        tag : AlvarMarker
        for tag in self.last_tags.markers:
            color = COLORS["tag"] 
            marker : Marker = self.get_marker((tag.pose.pose.position.x, tag.pose.pose.position.y, 0.), color, tag.tag_id, "tag")
            marker.header.stamp = self.last_tags.header.stamp
            marker.header.frame_id = self.last_tags.header.frame_id
            msg.markers.append(marker)
        self.publisher.publish(msg)

    def pub_last_planning_goal(self):
        """
        Publishes the last goals
        """
        msg = MarkerArray()
        goal = self.last_planning_goal
        color = COLORS["planning_goal"] 
        marker : Marker = self.get_marker((goal.pose.position.x, goal.pose.position.y, 0.), color, 0, "planning_goal")
        marker.scale.x = 0.3
        marker.scale.y = 0.3
        marker.scale.z = 0.1
        marker.header = self.last_planning_goal.header
        msg.markers.append(marker)
        self.publisher.publish(msg)

    def timer_callback(self):
        """
        Called every timer_period. Publishes to self.publisher
        :return:
        """
        if self.last_goals is not None:
            self.pub_last_goals()
        if self.last_tags is not None:
            self.pub_last_tags()
        if self.last_planning_goal is not None:
            self.pub_last_planning_goal()


def main():
    rclpy.init()
    template_node = TemplateNode()
    rclpy.spin(template_node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
