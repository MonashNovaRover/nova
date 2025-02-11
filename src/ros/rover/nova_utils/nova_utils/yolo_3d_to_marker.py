#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Republish yolo_ros Detection3DArray msg 
to Marker visualisation for cube localisation
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
 - Refine max_std_dev param
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
from rclpy.duration import Duration

from visualization_msgs.msg import MarkerArray, Marker
from geometry_msgs.msg import Pose, Point, Vector3, TransformStamped
from yolo_msgs.msg import DetectionArray, Detection, BoundingBox3D

from tf2_ros.transform_broadcaster import TransformBroadcaster
from tf2_ros import Buffer, TransformListener

import numpy as np

from typing import Dict, List, Tuple, TypeVar
T = TypeVar('T')

COLORS = {'red':[1.0,0.0,0.0], 'green':[0.0,1.0,0.0], 'blue':[0.0,0.0,1.0], 'white':[1.0,1.0,1.0]}
DEFAULT_QUATERNION = [0.0, 0.0, 0.0, 1.0]

DETECTION_TOPIC = "/detections_3d"
MARKER_TOPIC = "/cubes"
DETECTION_TYPES = ["detected", "confirmed"]
RVIZ_CUBE_SIZE = 0.15
MAP_FRAME = "map"
SIGNIFICANT_THRESHOLD = 0.01 # when standing still, the first 2 decimals shouldn't change, e.g 1.234 the 4th will be changing semi-frequently

class MarkerPublisher(Node):
    def __init__(self):
        super().__init__("cube_publisher")

        self.declare_parameter('namespace', '/yolo')
        self.frame = self.declare_parameter('target_frame', 'camera_link').get_parameter_value().string_value # the frame from which the detections are expected to originate from, same arg name as yolo to keep consistent
       
        self.min_samples = self.declare_parameter('min_samples', 5).get_parameter_value().integer_value
        self.max_std_dev = self.declare_parameter('max_std_dev', 0.2).get_parameter_value().double_value

        self.last_cubes : Dict[str, Tuple[float, float, float]] = {'red': None, 'green': None, 'blue': None, 'white': None}
        self.detected_cubes: Dict[str, List[Tuple[float, float, float]]] = {'red': [], 'green': [], 'blue': [], 'white': []}
        self.confirmed_cubes : Dict[str, Tuple[float, float, float]] = {'red': None, 'green': None, 'blue': None, 'white': None}

        self.transform_broadcaster = TransformBroadcaster(self)
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)

        self.publisher = self.create_publisher(
            MarkerArray,
            self.get_parameter('namespace').get_parameter_value().string_value + MARKER_TOPIC,
            10
        )

        self.subscription = self.create_subscription(
            DetectionArray,
            self.get_parameter('namespace').get_parameter_value().string_value + DETECTION_TOPIC, 
            self.callback, 
            10
        )

    def callback(self, msg: DetectionArray):
        """Callback for detection topic, adds detections to detected_cubes list"""
        
        self.current_detections : List[Tuple[str, Tuple[float, float, float]]] = []
        for detection in msg.detections:
            # correct the position of the cube
            # rotate 90 deg around x and -90 deg around z from camera_link
            # x = z, y = -x, z = -y (z not needed for confirming)
            point = (detection.bbox3d.center.position.z, -1*detection.bbox3d.center.position.x, -1*detection.bbox3d.center.position.y)

            # add cube point only if a significant movement has occurred
            if self.last_cubes[detection.class_name] is None or any(abs(c - p) > SIGNIFICANT_THRESHOLD for p, c in zip(self.last_cubes[detection.class_name], point)):
                corrected_point = self.tf_to_map(point, msg.header.stamp)
                self.detected_cubes[detection.class_name].append(corrected_point)
                self.last_cubes[detection.class_name] = point

            self.current_detections.append((detection.class_name, point))

        self.publish_detection(msg.header.stamp)
    

    def publish_detection(self, stamp) -> None:
        """Generates and publishes the markers."""
        msg = MarkerArray()
        det : Tuple[str, Tuple[float, float, float]]
        if self.current_detections is not None:
            for i, det in enumerate(self.current_detections):
                marker : Marker = self.get_marker(3+i, det[0], det[1], False, stamp, self.get_parameter('target_frame').get_parameter_value().string_value)
                msg.markers.append(marker)

        for i, color in enumerate(COLORS.keys()):
            has_new = self.attempt_confirm_target(color)
            if has_new:
                self.publish_tf(color, self.confirmed_cubes[color], stamp)
                marker : Marker = self.get_marker(i, color, self.confirmed_cubes[color], True, stamp, MAP_FRAME)
                msg.markers.append(marker)
        
        self.publisher.publish(msg)
        

    def publish_tf(self, color:str, position: Tuple[float,float,float], stamp) -> None:
        """Publish the transform of a confirmed cube"""
        tfs = TransformStamped()
        tfs.header.stamp = stamp
        tfs.header.frame_id = MAP_FRAME
        tfs.child_frame_id = color + "_cube"
        tfs.transform.translation.x,  tfs.transform.translation.y, tfs.transform.translation.z, = position
        tfs.transform.rotation.x, tfs.transform.rotation.y, tfs.transform.rotation.z, tfs.transform.rotation.w = DEFAULT_QUATERNION

        self.transform_broadcaster.sendTransform(tfs)

    def quaternion_to_rotation_matrix(self, q):
        """Convert a quaternion (x, y, z, w) into a 3x3 rotation matrix."""
        x, y, z, w = q
        return np.array([
            [1 - 2 * (y ** 2 + z ** 2), 2 * (x * y - z * w), 2 * (x * z + y * w)],
            [2 * (x * y + z * w), 1 - 2 * (x ** 2 + z ** 2), 2 * (y * z - x * w)],
            [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x ** 2 + y ** 2)]
        ])


    def tf_to_map(self, camera_to_cube: Tuple[float,float,float], stamp) -> Tuple[float, float, float]:
        """Calculates the position tf from map to cube"""
        map_to_camera = self.tf_buffer.lookup_transform(MAP_FRAME, self.frame, stamp).transform

        # rotate the position of the tf 
        rot_matrix = self.quaternion_to_rotation_matrix([map_to_camera.rotation.x,map_to_camera.rotation.y, map_to_camera.rotation.z, map_to_camera.rotation.w])
        rotated_translation = np.dot(rot_matrix, camera_to_cube)

        # apply the camera_to_cube tf to the rotated map_to_camera
        return (map_to_camera.translation.x+rotated_translation[0], map_to_camera.translation.y+rotated_translation[1], map_to_camera.translation.z+rotated_translation[2])

    def remove_outlier_pos(self, pos_vals: List[T]) -> List[T]:
        """
        Removes any outlier positions from the list of positions. An outlier is defined as a position that is
        more than 3 standard deviations away from the mean.
        """
        mean = np.mean(pos_vals, axis=0)
        std_dev = np.std(pos_vals, axis=0)

        return [pos for pos in pos_vals if np.all(np.abs(pos - mean) < 2 * std_dev)]


    def attempt_confirm_target(self, color:str) -> bool:
        """
        Checks that we have enough samples of this block, and that their position is sufficiently consistent
        to be considered a confirmed block.
        """
        target_pos = self.detected_cubes[color]
        if target_pos is not None and len(target_pos) >= self.min_samples:
            consistent_pos = self.remove_outlier_pos(target_pos)

        else:
            self.get_logger().debug(
                f"{color} has not enough samples to confirm target {color}")
            return False

        if len(consistent_pos) >= self.min_samples:
            self.get_logger().debug(f"Validating consistency of target {color}: {consistent_pos}")
            #target_pos_vals = consistent_pos[-self.min_samples:] not sure what this line is for

            # We have enough samples to be confident in this block's position
            # Calculate the average position of the block
            avg_pos = np.mean(consistent_pos, axis=0)
            # Calculate the standard deviation of the block's position
            std_dev = np.std(consistent_pos, axis=0)
            # Check that the standard deviation is small enough to be considered a confirmed block
            if np.all(std_dev < self.max_std_dev):
                self.get_logger().debug(f"Confirmed target {color} consistent pos at {avg_pos}")
                # We have a confirmed block
                if color is not None:
                    self.confirmed_cubes[color] = avg_pos
                return True
            else:
                print(consistent_pos)
                self.get_logger().debug(f"Target {color} is not consistent enough: {consistent_pos}")
                return False

    def get_marker(self, id:int, color:str, point: Tuple[float, float, float], confirmed: bool, stamp, frame:str) -> Marker:
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

        marker.lifetime = Duration(seconds=0).to_msg() if confirmed else Duration(seconds=0.3).to_msg()
        marker.ns = DETECTION_TYPES[1] if confirmed else DETECTION_TYPES[0]
        marker.id = id

        marker.header.stamp = stamp
        marker.header.frame_id = frame

        return marker


def main():
    rclpy.init()
    node = MarkerPublisher()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == "__main__":
    main()
