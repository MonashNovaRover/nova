#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Republish yolo_ros Detection3DArray msg 
to Marker visualisation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: yolo_3d_to_marker
TOPICS:
  - subscriber: /yolo/detections_3d [yolo_msgs/msg/DetectionArray]
  - publisher: /template/publisher [String]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_utils
AUTHOR(S):	Anthony Lew
CREATION:	10/02/2025
EDITED:		10/02/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - change the template
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.duration import Duration

from visualization_msgs.msg import MarkerArray, Marker
from geometry_msgs.msg import Pose, Vector3

from yolo_msgs import DetectionArray, Detection, BoundingBox3D

COLORS = ['red', 'green', 'blue', 'white']

SUBSCRIBE_TOPIC = "/yolo/detections_3d"
PUBLISHER_MARKER_TOPIC = "/yolo/cubes/markers"
PUBLISHER_TEXT_TOPIC = "/yolo/cubes/text"

PERIOD = 0.1 # in seconds

class MarkerPublisher(Node):
    def __init__(self):
        super().__init__("marker_publisher")
        self.sub_goals = self.create_subscription(Detection3DArray, SUBSCRIBE_TOPIC, self.cb, 10)

        self.publisher = self.create_publisher(MarkerArray, PUBLISHER_MARKER_TOPIC, 10)
        self.publisher_text = self.create_publisher(MarkerArray, PUBLISHER_TEXT_TOPIC, 10)

        self.detections : DetectionArray = None

        self.create_timer(PERIOD, self.timer_callback)

    def cb(self, msg: DetectionArray):
        self.detections = msg

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
        Called every period. Publishes to self.publisher
        :return:
        """
        self.pub()

def main():
    rclpy.init()
    node = MarkerPublisher()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
