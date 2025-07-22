#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Republish OAKD NN Detection3DArray msg to
Marker visusalisation
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

from visualization_msgs.msg import MarkerArray, Marker
from vision_msgs.msg import Detection3DArray, Detection3D
from geometry_msgs.msg import PoseStamped

COLORS = {
    "0": (0.0, 0.0, 0.9),
    "1": (0.0, 0.0, 0.0),
    "2": (0.0, 0.9, 0.0),
    "3": (0.96, 0.66, 0.05),
    "4": (1.0, 1.0, 1.0),
    "5": (0.9, 0.88, 0.12)
}

class MarkerPublisher(Node):
    def __init__(self):
        super().__init__("marker_publisher")
        self.sub_goals = self.create_subscription(Detection3DArray, "/oak/nn/spatial_detections", self.cb, 10)

        self.publisher = self.create_publisher(MarkerArray, "/oak/nn/spatial_detections_markers", 10)
        self.publisher_text = self.create_publisher(MarkerArray, "/oak/nn/spatial_detections_text", 10)

        self.last_detections : Detection3DArray = None

        timer_period = 0.1  # run the timer 10 times per second
        self.create_timer(timer_period, self.timer_callback)

    def cb(self, msg):
        self.last_detections = msg

    def get_marker(self, point, c: tuple, index: int, type_) -> None:
        """
        :params: c is color tuple (r,g,b) between 0 and 1
        """
        msg = Marker()
        msg.pose.position.x = point[0]
        msg.pose.position.y = point[1]
        msg.pose.position.z = point[2]
        msg.pose.orientation.w = 1.0
        msg.type = type_
        msg.scale.x = .15
        msg.scale.y = .15
        msg.scale.z = .15
        msg.color.r = c[0]
        msg.color.g = c[1]
        msg.color.b = c[2]
        msg.color.a = 1.

        if type_ == Marker.TEXT_VIEW_FACING:
            msg.text = f'{point[0]:.2f}' + "," + f'{point[1]:.2f}'

        msg.lifetime = Duration(seconds=0.2).to_msg()
        # Namespace - raw messages can be separated from confirmed cubes
        #msg.ns = namespace
        msg.id = index
        # Survive for half a second
        return msg

    def pub(self):
        """
        Publishes the markers
        """
        msg = MarkerArray()
        msg_text = MarkerArray()
        det : Detection3D
        if self.last_detections != None:
            for i, det in enumerate(self.last_detections.detections):
                #print("det [pub]: ", det)
                det_pose = det.results[0].pose.pose
                marker : Marker = self.get_marker((det_pose.position.x, det_pose.position.y, 0.), COLORS[det.results[0].hypothesis.class_id], i, Marker.CUBE)
                marker_text : Marker = self.get_marker((det_pose.position.x, det_pose.position.y, 0.4), COLORS[det.results[0].hypothesis.class_id], i, Marker.TEXT_VIEW_FACING)

                marker.header.frame_id = self.last_detections.header.frame_id
                marker_text.header.frame_id = self.last_detections.header.frame_id
                msg.markers.append(marker)
                msg_text.markers.append(marker_text)

        self.publisher.publish(msg)
        self.publisher_text.publish(msg_text)

    def timer_callback(self):
        """
        Called every timer_period. Publishes to self.publisher
        :return:
        """
        #if self.last_detections is not None:
        #   self.pub()
        self.pub()

def main():
    rclpy.init()
    node = MarkerPublisher()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
