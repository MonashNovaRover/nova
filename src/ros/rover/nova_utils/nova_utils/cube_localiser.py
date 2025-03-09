#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Republish yolo_ros DetectionArray msg 
as a transform for cube localisation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: yolo_3d_to_marker
TOPICS:
  - subscriber: /yolo/detections           [yolo_msgs/msg/DetectionArray]
  - subscriber: /oak/depth                 [sensor_msgs/msg/Image]
  - subscriber: /oak/camera_info           [sensor_msgs/msg/CameraInfo]
  - subscriber: /oak/nn/spatial_detections [vision_msgs/msg/Detection3DArray]
  - publisher: /yolo/cubes                 [visualization_msgs/msg/MarkerArray]
  - publisher: /tf                         [geometry_msgs/msg/TransformStamped]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_utils
AUTHOR(S):	Anthony Lew
CREATION:	20/02/2025
EDITED:		20/02/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Refine the statistical analysis
 - Refine the standard dev and min samples params
 - Convert to work for oak camera
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
import rclpy
from rclpy.node import Node
from rclpy.duration import Duration
from rclpy.qos import QoSHistoryPolicy, QoSDurabilityPolicy, QoSProfile

from visualization_msgs.msg import MarkerArray, Marker
from geometry_msgs.msg import Pose, Vector3, TransformStamped
from sensor_msgs.msg import Image, CameraInfo
from builtin_interfaces.msg import Time

# im pretty sure these are almost identical in how we use them except for getting the colour of the name
# vision_msgs hides the class_name behind a class_id
from yolo_msgs.msg import DetectionArray, Detection, BoundingBox2D 
from vision_msgs.msg import Detection3DArray, Detection2DArray, Detection2D, BoundingBox2D


from tf2_ros.transform_broadcaster import TransformBroadcaster
from tf2_ros import Buffer, TransformListener
from cv_bridge import CvBridge

import message_filters

import cv2
import numpy as np

from typing import Dict, List, Tuple, TypeVar
T = TypeVar('T')
type Point = Tuple[float, float, float] # point = (x,y,z)


COLORS = {'red':[1.0,0.0,0.0], 'green':[0.0,1.0,0.0], 'blue':[0.0,0.0,1.0], 'white':[1.0,1.0,1.0]}
DEFAULT_QUATERNION = [0.0, 0.0, 0.0, 1.0]

# Assumed color ids:
IDS_COLOR = {0: 'red', 1: 'green', 2: 'blue', 3: 'white'} 
# could replace with an array ['red', 'green', 'blue', 'white'] as indexes are implicit, but this is better for readability

# change these topics using remapping in launch file
DEPTH_IMAGE_TOPIC = 'image_raw'
DEPTH_INFO_TOPIC = 'camera_info'
DETECTION_TOPIC = '/oak/nn/spatial_detections'
MARKER_TOPIC = 'cubes'


class DetectionTransformer(Node):
    def __init__(self):
        super().__init__('detection_transformer')

        # variables affecting detection of cubes
        self.depth_image_units_divisor = self.declare_parameter('depth_image_units_divisor', 1.0).get_parameter_value().double_value
        self.maximum_detection_threshold = self.declare_parameter('maximum_detection_threshold', 0.3).get_parameter_value().double_value
        self.significant_threshold = self.declare_parameter('significant_threshold', 0.01).get_parameter_value().double_value

        # variables affecting cube markers in rviz
        self.use_markers = self.declare_parameter('use_markers', True).get_parameter_value().bool_value
        self.marker_ns = self.declare_parameter('marker_ns', 'detected_cubes').get_parameter_value().string_value
        self.marker_duration = self.declare_parameter('marker_duration', 1.0).get_parameter_value().double_value
        self.marker_size = self.declare_parameter('marker_size', 0.15).get_parameter_value().double_value

        # variables to determine frame of map and camera
        self.map_frame = self.declare_parameter('map_frame', 'map').get_parameter_value().string_value
        self.camera_frame = self.declare_parameter('camera_frame', 'camera_link').get_parameter_value().string_value

        # timer period to publish cube transforms in seconds
        self.tf_publisher_timer_period = self.declare_parameter('tf_publisher_timer_period', 0.1).get_parameter_value().double_value

        # variables for statistical analysis
        self.min_samples = self.declare_parameter('min_samples', 5).get_parameter_value().integer_value
        self.max_std_dev = self.declare_parameter('max_std_dev', 0.2).get_parameter_value().double_value

        self.using_yolo_ros = self.declare_parameter('sim', True).get_parameter_value().bool_value

        self.transform_broadcaster = TransformBroadcaster(self)
        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)

        self.cv_bridge = CvBridge()

        self.default_qos_profile = QoSProfile(
            reliability=1,
            history=QoSHistoryPolicy.KEEP_LAST,
            durability=QoSDurabilityPolicy.VOLATILE,
            depth=1,
        )

        # subscribe to the topics
        self.depth_sub = message_filters.Subscriber(
            self, Image, DEPTH_IMAGE_TOPIC, qos_profile=self.default_qos_profile
        )
        self.depth_info_sub = message_filters.Subscriber(
            self, CameraInfo, DEPTH_INFO_TOPIC, qos_profile=self.default_qos_profile
        )

        if DETECTION_TOPIC == '/yolo/detections':
            DETECTION_TYPE = DetectionArray
        elif DETECTION_TOPIC == '/oak/nn/detections':
            DETECTION_TYPE = Detection2DArray
        elif DETECTION_TOPIC == '/oak/nn/spatial_detections':
            DETECTION_TYPE = Detection3DArray
        
        self.detections_sub = message_filters.Subscriber(
            self, DETECTION_TYPE, DETECTION_TOPIC
        )
        # synchronise information from topics and run function upon all information received
        self._synchronizer = message_filters.ApproximateTimeSynchronizer(
            (self.depth_sub, self.depth_info_sub, self.detections_sub), 10, 0.5)
        self._synchronizer.registerCallback(self.on_detections)

        # publish to the marker topic
        self.publisher = self.create_publisher(MarkerArray, MARKER_TOPIC, 10)

        self.detected_cubes : Dict[str, List[Point]] \
            = {'red':[], 'green':[], 'blue':[], 'white':[]}
        
        # run the callback function every timer_period
        self.create_timer(self.tf_publisher_timer_period, self.publish_cubes)


    def on_detections(self, depth_msg: Image, depth_info_msg: CameraInfo, detections_msg: DetectionArray | Detection3DArray) -> None:
        '''Process and publish detected cubes'''
        detections = self.process_detections(depth_msg, depth_info_msg, detections_msg)

        # Markers
        if (self.use_markers):
            msg = MarkerArray()
            detection: Tuple[str, Point]
            for i, detection in enumerate(detections):
                marker = self.get_marker(i, detection[0], detection[1], detections_msg.header.stamp, self.map_frame)
                msg.markers.append(marker)
            
            self.publisher.publish(msg)
        
        # Transforms
        detection: Tuple[str, Point]
        for i, detection in enumerate(detections):
            self.detected_cubes[detection[0]].append(detection[1])

    def publish_cubes(self) -> None:
        '''Publish the current transform of all detected cubes'''
        for color, points in self.detected_cubes.items():
            if len(points) >= self.min_samples:
                clean_points = self.remove_outlier_pos(points)  # remove outliers from all the cube points

                if len(clean_points) >= self.min_samples:       # ensure there is enough samples
                    self.get_logger().debug(f'Validating consistency of target {color}')
                    avg_pos = np.mean(clean_points, axis=0)   # Calculate the average position of the block
                    std_dev = np.std(clean_points, axis=0)    # Calculate the standard deviation of the block's position

                    if np.all(std_dev < self.max_std_dev):      # Check that the standard deviation is small enough to be considered a confirmed block
                        self.get_logger().debug(f'Confirmed target {color} consistent pos at {avg_pos}')
                        self.publish_tf(color, avg_pos, self.get_clock().now().to_msg())
                    else:
                        self.get_logger().debug(f'Target {color} is not consistent enough.')
                else:
                    self.get_logger().debug(f'{color} has not enough samples to confirm')
                

    def process_detections(self, depth_msg: Image, depth_info_msg: CameraInfo, detections_msg: DetectionArray | Detection3DArray) -> List[Tuple[str,Point]]:
        '''Process detections into a list of points with their respective colour'''
        if not detections_msg.detections:
            return []

        self.get_logger().debug(f'Processing new detections')

        new_detections = []

        if self.using_yolo_ros:
            depth_image = self.cv_bridge.imgmsg_to_cv2(depth_msg, desired_encoding='32FC1')

            for detection in detections_msg.detections:
                position = self.convert_bb_to_point(depth_image, depth_info_msg, detection)
                if position is not None:
                    new_position = self.tf_to_map(position, detections_msg.header.stamp)
                    if new_position is not None:
                        # Detection will be of Detection from yolo_msgs
                        color:str = detection.class_name
                        new_detections.append((color, new_position))
        else:
            for detection in detections_msg.detections:
                for result in detection.results:
                    position = (float(result.pose.pose.position.x), float(result.pose.pose.position.y), float(result.pose.pose.position.z))
                    new_position = self.tf_to_map(position, detections_msg.header.stamp)
                    if new_position is not None:
                        # Detection will be of Detection3D from vision_msgs
                        color:str = IDS_COLOR[int(result.hypothesis.class_id)]
                        new_detections.append((color, new_position))

        return new_detections

    def convert_bb_to_point(self, depth_image: np.ndarray, depth_info: CameraInfo, detection: Detection) -> Point | None:
        ''' Converts the bounding box center to a point relative to the image frame
            Modified from convert_bb_to_3d in https://github.com/mgonzs13/yolo_ros/blob/main/yolo_ros/yolo_ros/detect_3d_node.py
        '''
        self.get_logger().debug(f'Calculating cube world position relative to image frame')
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

    def quaternion_to_rotation_matrix(self, q: Tuple[float, float, float, float]) -> np.array:
        '''Convert a quaternion (x, y, z, w) into a 3x3 rotation matrix.'''
        x, y, z, w = q
        return np.array([
            [1 - 2 * (y ** 2 + z ** 2), 2 * (x * y - z * w), 2 * (x * z + y * w)],
            [2 * (x * y + z * w), 1 - 2 * (x ** 2 + z ** 2), 2 * (y * z - x * w)],
            [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x ** 2 + y ** 2)]
        ])

    def tf_to_map(self, camera_to_cube: Point, stamp: Time) -> Point | None:
        '''Calculates the position tf from map to cube'''
        self.get_logger().debug(f'Calculating map to cube tf')
        try:
            map_to_camera = self.tf_buffer.lookup_transform(self.map_frame, self.camera_frame, stamp).transform
            # rotate the map_to_camera position by its orientation 
            rot_matrix = self.quaternion_to_rotation_matrix([map_to_camera.rotation.x, map_to_camera.rotation.y, map_to_camera.rotation.z, map_to_camera.rotation.w])
            rotated_translation = np.dot(rot_matrix, camera_to_cube)
            # apply the camera_to_cube tf to the rotated map_to_camera
            return (map_to_camera.translation.x+rotated_translation[0], map_to_camera.translation.y+rotated_translation[1], map_to_camera.translation.z+rotated_translation[2])
        except Exception as e:
            self.get_logger().warn(f'Error in calculating map to cube tf {e}')
            return None

    def get_marker(self, id:int, color:str, point: Point, stamp: Time, frame:str) -> Marker:
        '''Returns a marker derived from the detection'''
        marker = Marker()
        marker.pose.position.x, marker.pose.position.y, marker.pose.position.z = point
        marker.pose.orientation.x, marker.pose.orientation.y, marker.pose.orientation.z, marker.pose.orientation.w = DEFAULT_QUATERNION

        marker.type = Marker.CUBE
        marker.scale.x = self.marker_size
        marker.scale.y = self.marker_size
        marker.scale.z = self.marker_size
        marker.color.r = COLORS[color][0]
        marker.color.g = COLORS[color][1]
        marker.color.b = COLORS[color][2]
        marker.color.a = 1.0

        marker.lifetime = Duration(seconds=self.marker_duration).to_msg()
        marker.ns = self.marker_ns
        marker.id = id

        marker.header.stamp = stamp
        marker.header.frame_id = frame

        return marker

    def remove_outlier_pos(self, pos_vals: List[T]) -> List[T]:
        '''
        Removes any outlier positions from the list of positions. An outlier is defined as a position that is
        more than 3 standard deviations away from the mean.
        '''
        mean = np.mean(pos_vals, axis=0)
        std_dev = np.std(pos_vals, axis=0)

        return [pos for pos in pos_vals if np.all(np.abs(pos - mean) < 2 * std_dev)]

    def publish_tf(self, color:str, position: Point, stamp: Time) -> None:
        '''Publish the transform of a confirmed cube'''
        tfs = TransformStamped()
        tfs.header.stamp = stamp
        tfs.header.frame_id = self.map_frame
        tfs.child_frame_id = color + '_cube'
        tfs.transform.translation.x,  tfs.transform.translation.y, tfs.transform.translation.z, = position
        tfs.transform.rotation.x, tfs.transform.rotation.y, tfs.transform.rotation.z, tfs.transform.rotation.w = DEFAULT_QUATERNION

        self.transform_broadcaster.sendTransform(tfs)






def main():
    rclpy.init()
    node = DetectionTransformer()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()