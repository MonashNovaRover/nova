#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Republish yolo_ros DetectionArray msg 
as a transform for cube localisation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: yolo_3d_to_marker
TOPICS:
  - subscriber: /yolo/detections [yolo_msgs/msg/DetectionArray]
  - subscriber: /oak/depth/points [sensor_msgs/msg/PointCloud2]
  - publisher: /yolo/cubes [visualization_msgs.MarkerArray]
  - publisher: /tf
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_utils
AUTHOR(S):	Anthony Lew
CREATION:	20/02/2025
EDITED:		20/02/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Refine max_std_dev param
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.duration import Duration
from rclpy.qos import QoSHistoryPolicy, QoSDurabilityPolicy, QoSProfile

from visualization_msgs.msg import MarkerArray, Marker
from geometry_msgs.msg import Pose, Point, Vector3, TransformStamped
from yolo_msgs.msg import DetectionArray, Detection, BoundingBox3D
from sensor_msgs.msg import Image, CameraInfo

from tf2_ros.transform_broadcaster import TransformBroadcaster
from tf2_ros import Buffer, TransformListener
from cv_bridge import CvBridge

import message_filters

import cv2
import numpy as np

from typing import Dict, List, Tuple, TypeVar
T = TypeVar('T')

COLORS = {'red':[1.0,0.0,0.0], 'green':[0.0,1.0,0.0], 'blue':[0.0,0.0,1.0], 'white':[1.0,1.0,1.0]}
DEFAULT_QUATERNION = [0.0, 0.0, 0.0, 1.0]
RVIZ_CUBE_SIZE = 0.15

DETECTION_TOPIC = "/detections"
MARKER_TOPIC = "/cubes"
DETECTION_TYPES = ["detected"]
TARGET_FRAME = "map"

class DetectionTransformer(Node):
    def __init__(self):
        super().__init__("detection_transformer")
        namespace = self.get_namespace() 
        namespace = "/yolo"

        self.declare_parameter('target_frame', 'map')
        self.depth_image_units_divisor = 1
        self.maximum_detection_threshold = 0.3

        self.transform_broadcaster = TransformBroadcaster(self)
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)

        self.cv_bridge = CvBridge()

        self.publisher = self.create_publisher(
            MarkerArray,
            namespace + MARKER_TOPIC,
            10
        )

        self.default_qos_profile = QoSProfile(
            reliability=1,
            history=QoSHistoryPolicy.KEEP_LAST,
            durability=QoSDurabilityPolicy.VOLATILE,
            depth=1,
        )

        self.depth_sub = message_filters.Subscriber(
            self, Image, "/oak/depth", qos_profile=self.default_qos_profile
        )
        self.depth_info_sub = message_filters.Subscriber(
            self, CameraInfo, "/oak/camera_info", qos_profile=self.default_qos_profile
        )
        self.detections_sub = message_filters.Subscriber(
            self, DetectionArray, "/yolo/detections"
        )

        self._synchronizer = message_filters.ApproximateTimeSynchronizer(
            (self.depth_sub, self.depth_info_sub, self.detections_sub), 10, 0.5
        )
        self._synchronizer.registerCallback(self.on_detections)

    def on_detections(self, depth_msg: Image, depth_info_msg: CameraInfo, detections_msg: DetectionArray) -> None:
        detections = self.process_detections(depth_msg, depth_info_msg, detections_msg)

        msg = MarkerArray()

        for i, detection in enumerate(detections):
            marker = self.get_marker(i, detection[0], detection[1], detections_msg.header.stamp, "camera_link")
            msg.markers.append(marker)
        
        self.publisher.publish(msg)


    def process_detections(self, depth_msg: Image, depth_info_msg: CameraInfo, detections_msg: DetectionArray) -> List[Tuple[str,Point]]:
        # check if there are detections
        if not detections_msg.detections:
            return []

        new_detections = []
        depth_image = self.cv_bridge.imgmsg_to_cv2(depth_msg, desired_encoding="32FC1")

        for detection in detections_msg.detections:
            position = self.convert_bb_to_point(depth_image, depth_info_msg, detection)
            if position is not None:
                # transform point from image frame to map frame 
                new_detections.append((detection.class_name, position))

        return new_detections

    def convert_bb_to_point(self, depth_image: np.ndarray, depth_info: CameraInfo, detection: Detection) -> Point | None:
        """Converts the bounding box center to a point relative to the image frame"""

        center_x = int(detection.bbox.center.position.x)
        center_y = int(detection.bbox.center.position.y)
        size_x = int(detection.bbox.size.x)
        size_y = int(detection.bbox.size.y)

        # crop depth image by the 2d BB
        u_min = max(center_x - size_x // 2, 0)
        u_max = min(center_x + size_x // 2, depth_image.shape[1] - 1)
        v_min = max(center_y - size_y // 2, 0)
        v_max = min(center_y + size_y // 2, depth_image.shape[0] - 1)

        roi = depth_image[v_min:v_max, u_min:u_max]

        roi = roi / self.depth_image_units_divisor  # convert to meters
        if not np.any(roi):
            return None

        # find the z coordinate on the 3D BB
        bb_center_z_coord = (
            depth_image[int(center_y)][int(center_x)] / self.depth_image_units_divisor
        )

        z_diff = np.abs(roi - bb_center_z_coord)
        mask_z = z_diff <= self.maximum_detection_threshold
        if not np.any(mask_z):
            return None

        roi = roi[mask_z]
        z_min, z_max = np.min(roi), np.max(roi)
        z = (z_max + z_min) / 2

        if z == 0:
            return None

        # project from image to world space
        k = depth_info.k
        px, py, fx, fy = k[2], k[5], k[0], k[4]
        x = z * (center_x - px) / fx
        y = z * (center_y - py) / fy
        #w = z * (size_x / fx)
        #h = z * (size_y / fy)
        #d = float(z_max - z_min)

        # rotate 90 deg around x and -90 deg around z from camera_link
        return (z, -1*x, -1*y)

    def get_marker(self, id:int, color:str, point: Tuple[float, float, float], stamp, frame:str) -> Marker:
        """Returns a marker derived from the detection"""
        marker = Marker()
        marker.pose.position.x, marker.pose.position.y, marker.pose.position.z = point
        marker.pose.orientation.x, marker.pose.orientation.y, marker.pose.orientation.z, marker.pose.orientation.w = DEFAULT_QUATERNION

        marker.type = Marker.CUBE
        marker.scale.x = RVIZ_CUBE_SIZE
        marker.scale.y = RVIZ_CUBE_SIZE
        marker.scale.z = RVIZ_CUBE_SIZE
        marker.color.r = COLORS[color][0]
        marker.color.g = COLORS[color][1]
        marker.color.b = COLORS[color][2]
        marker.color.a = 1.0

        marker.lifetime = Duration(seconds=0.3).to_msg()
        marker.ns = "detected"
        marker.id = id

        marker.header.stamp = stamp
        marker.header.frame_id = frame

        return marker



def main():
    rclpy.init()
    node = DetectionTransformer()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
