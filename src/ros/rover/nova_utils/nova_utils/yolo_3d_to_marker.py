#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Republish yolo_ros Detection3DArray msg 
to Marker visualisation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: yolo_3d_to_marker
TOPICS:
  - subscriber: /yolo/detections_3d [yolo_msgs/msg/DetectionArray]
  - publisher: /yolo/cubes/markers [visualization_msgs.MarkerArray]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_utils
AUTHOR(S):	Anthony Lew
CREATION:	10/02/2025
EDITED:		10/02/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Confirm boxes on map, base on cube_tracker.py by Max Tory
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.duration import Duration

from visualization_msgs.msg import MarkerArray, Marker
from geometry_msgs.msg import Pose, Vector3

from yolo_msgs.msg import DetectionArray, Detection, BoundingBox3D

COLORS = {'red':[1.0,0.0,0.0], 'green':[0.0,1.0,0.0], 'blue':[0.0,0.0,1.0], 'white':[1.0,1.0,1.0]}

DETECTION_TOPIC = "/detections_3d"
MARKER_TOPIC = "/cubes"
DETECTION_TYPES = ["detected", "confirmed"]
RVIZ_CUBE_SIZE = 0.15

class MarkerPublisher(Node):
    def __init__(self):
        super().__init__("marker_publisher")
        self.declare_parameter('namespace', '/yolo')
        self.subscription = self.create_subscription(
            DetectionArray,
            self.get_parameter('namespace').get_parameter_value().string_value + DETECTION_TOPIC, 
            self.callback, 
            10
        )
        self.publisher = self.create_publisher(
            MarkerArray,
            self.get_parameter('namespace').get_parameter_value().string_value + MARKER_TOPIC,
            10
        )

        self.detection_array: DetectionArray = None

    def callback(self, msg: DetectionArray):
        self.detection_array = msg
        self.publish()

    def get_marker(self, detection: Detection) -> None:
        """Returns a marker derived from the detection"""
        marker = Marker()
        marker.pose = detection.bbox3d.center
        # rotate 90 deg around x and -90 deg around z from camera_link
        marker.pose.position.x, marker.pose.position.y, marker.pose.position.z = detection.bbox3d.center.position.z, -1*detection.bbox3d.center.position.x, -1*detection.bbox3d.center.position.y
        marker.type = Marker.CUBE
        marker.scale.x = RVIZ_CUBE_SIZE
        marker.scale.y = RVIZ_CUBE_SIZE
        marker.scale.z = RVIZ_CUBE_SIZE
        marker.color.r = COLORS[detection.class_name][0]
        marker.color.g = COLORS[detection.class_name][1]
        marker.color.b = COLORS[detection.class_name][2]
        marker.color.a = 1.0

        marker.lifetime = Duration(seconds=0.3).to_msg()
        marker.ns = DETECTION_TYPES[0]
        marker.id = detection.class_id
        return marker

    def publish(self):
        """Generates and publishes the markers."""
        msg = MarkerArray()
        det : Detection
        if self.detection_array is not None:
            for det in self.detection_array.detections:
                marker : Marker = self.get_marker(det)

                marker.header = self.detection_array.header
                msg.markers.append(marker)

        self.publisher.publish(msg)

def main():
    rclpy.init()
    node = MarkerPublisher()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
