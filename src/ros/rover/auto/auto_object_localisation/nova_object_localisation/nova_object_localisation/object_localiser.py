#!/usr/bin/env python3
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Republish OAK's DetectionArray msgs 
as a transform for object localisation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: object_localiser
TOPICS:
  - subscriber: /yolo/detections    [yolo_msgs/msg/DetectionArray]
  - subscriber: /oak/nn/detections  [vision_msgs/msg/Detection2DArray]
  - subscriber: /oak/depth          [sensor_msgs/msg/Image]
  - subscriber: /oak/camera_info    [sensor_msgs/msg/CameraInfo]
  - publisher: /yolo/objects        [visualization_msgs/msg/MarkerArray]
  - publisher: /tf                  [geometry_msgs/msg/TransformStamped]
SERVICES: None
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_utils
AUTHOR(S):	Anthony Lew, Chetan Edupalli
CREATION:	20/02/2025
EDITED:		04/03/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Check for errors after replacing cube with object and colour with label in code lol
 - Reenable /tf publishing
 - Refine the statistical analysis
 - Refine the standard dev and min samples params
 - Make 3D mode work again (e.g using spatial on OAK and 3d node with yolo_ros)
 - Add that thing about map boundaries (look in archive code)
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
import rclpy
from rclpy.node import Node
from rclpy.duration import Duration
from rclpy.qos import QoSHistoryPolicy, QoSDurabilityPolicy, QoSProfile

from visualization_msgs.msg import MarkerArray, Marker
from geometry_msgs.msg import Pose, Vector3, TransformStamped, PointStamped
from sensor_msgs.msg import Image, CameraInfo
from builtin_interfaces.msg import Time

# yolo_msgs used for yolo_ros, vision_msgs used for OAK camera (whichever one YOLO model runs on)
from yolo_msgs.msg import DetectionArray, Detection, BoundingBox2D 
from vision_msgs.msg import Detection3DArray, Detection3D, ObjectHypothesisWithPose, Detection2DArray, Detection2D, BoundingBox2D


from tf2_ros.transform_broadcaster import TransformBroadcaster
from tf2_ros import Buffer, TransformListener
from cv_bridge import CvBridge

from sensor_msgs.msg import PointCloud2

import message_filters
import tf2_geometry_msgs

import cv2
import numpy as np

from typing import Dict, List, Tuple, TypeVar
T = TypeVar('T')
type Point = Tuple[float, float, float]         # point = (x,y,z)
type BBox = Tuple[float, float, float, float]   # bounding_box = (pos_x, pos_y, size_x, size_y)
type ObjectPoint = Tuple[str, Point]            # object_point = (label, Point)


# LABELS = {'red':[1.0,0.0,0.0], 'green':[0.0,1.0,0.0], 'blue':[0.0,0.0,1.0], 'white':[1.0,1.0,1.0]} # ARCh 2025
LABELS = {'bottle':[0.0,0.0,1.0], 'mallet':[1.0,0.0,0.0]} # URC 2025
DEFAULT_QUATERNION = [0.0, 0.0, 0.0, 1.0]

# Object ids:
# Note: This has the same order as mappings in the generated .json file
# IDS_LABEL = { 0: 'blue', 1: 'green', 2: 'red', 3: 'white'} # ARCh 2025
IDS_LABEL = { 0: 'bottle', 1: 'mallet'} # URC 2025


class ObjectLocaliser(Node):
    def __init__(self):
        super().__init__('object_localiser')

        # variables affecting detection of objects
        self.depth_image_units_divisor = self.declare_parameter('depth_image_units_divisor', 1.0).get_parameter_value().double_value        # Used to calculate position from bounding box
        self.maximum_detection_threshold = self.declare_parameter('maximum_detection_threshold', 0.3).get_parameter_value().double_value    # any detections with a depth below this will be ignored (used to convert bb to point)
        self.scale_factor = [self.declare_parameter('x_scalar', 1.0).get_parameter_value().double_value,
                             self.declare_parameter('y_scalar', 1.0).get_parameter_value().double_value]


        # variables affecting object markers in rviz
        self.use_markers = self.declare_parameter('use_markers', True).get_parameter_value().bool_value
        self.marker_ns = self.declare_parameter('marker_ns', 'detected_objects').get_parameter_value().string_value
        self.marker_duration = self.declare_parameter('marker_duration', 1.0).get_parameter_value().double_value
        self.marker_size = self.declare_parameter('marker_size', 0.15).get_parameter_value().double_value

        # variables to determine frame of map and camera
        self.map_frame = self.declare_parameter('map_frame', 'map').get_parameter_value().string_value
        self.camera_frame = self.declare_parameter('camera_frame', 'oak_link').get_parameter_value().string_value

        # timer period to publish object transforms in seconds
        self.tf_publisher_timer_period = self.declare_parameter('tf_publisher_timer_period', 0.1).get_parameter_value().double_value

        # variables for statistical analysis
        self.min_samples = self.declare_parameter('min_samples', 5).get_parameter_value().integer_value
        self.max_std_dev = self.declare_parameter('max_std_dev', 0.2).get_parameter_value().double_value

        # # variables determining the mode of use
        # self.using_oak = self.declare_parameter('using_oak', True).get_parameter_value().bool_value
        # self.using_3d = self.declare_parameter('using_3d', False).get_parameter_value().bool_value

        # variables for compatibility with different detection message types
        self.use_vision_msgs = self.declare_parameter('use_vision_msgs', True).value
        self.using_3d = self.declare_parameter('using_3d', False).value
        self.use_pointcloud = self.declare_parameter('use_pointcloud', False).value

        # Topics to subscribe to:
        self.depth_info_topic = self.declare_parameter('depth_info_topic', '/camera/camera_info').value
        self.depth_image_topic = self.declare_parameter('depth_image_topic', '/camera/depth/image_raw').value
        self.pointcloud_topic = self.declare_parameter('pointcloud_topic', '/camera/depth/color/points').value
        self.detection_topic = self.declare_parameter('detection_topic', '/detections').value
        # Topic to publish to:
        self.marker_topic = self.declare_parameter('marker_topic', '/yolo/objects').value

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

        # subscribe and synchronise depth image topics if not using 3D
        if not self.using_3d:
            self.depth_info_sub = message_filters.Subscriber(self, CameraInfo, self.depth_info_topic, qos_profile=self.default_qos_profile)
            detection_type = Detection2DArray if self.use_vision_msgs else DetectionArray
            self.detections_sub = message_filters.Subscriber(self, detection_type, self.detection_topic)

            if self.use_pointcloud:
                self.pc_sub = message_filters.Subscriber(self, PointCloud2, self.pointcloud_topic, qos_profile=self.default_qos_profile)
                self._synchronizer = message_filters.ApproximateTimeSynchronizer((self.pc_sub, self.depth_info_sub, self.detections_sub), 10, 0.5)
                self._synchronizer.registerCallback(self.on_detections_pc)
                self.get_logger().info(f"Using PointCloud topic: {self.pointcloud_topic}")
            else:
                self.depth_sub = message_filters.Subscriber(self, Image, self.depth_image_topic, qos_profile=self.default_qos_profile)
                self._synchronizer = message_filters.ApproximateTimeSynchronizer((self.depth_sub, self.depth_info_sub, self.detections_sub), 10, 0.5)
                self._synchronizer.registerCallback(self.on_detections)
                self.get_logger().info(f"Using depth image topic: {self.depth_image_topic}")

        # otherwise just subscribe to DetectionArray
        else:
            detection_type = Detection3DArray if self.use_vision_msgs else DetectionArray
            self.detections_sub = self.create_subscription(detection_type, self.detection_topic, self.on_detections_3d, 10)
            self.get_logger().info(f"Reusing pose from detection topic: {self.detection_topic}")

        # publish to the marker topic
        if self.use_markers:
            self.publisher = self.create_publisher(MarkerArray, self.marker_topic, 10)
            self.get_logger().info(f"[{self.get_name()}] Publishing markers to: {self.marker_topic}")

        self.detected_objects : Dict[str, List[Point]] \
            = {label:[] for label in LABELS}

        # run the callback function every timer_period
        self.create_timer(self.tf_publisher_timer_period, self.publish_objects)

        self.get_logger().info(f"[{self.get_name()}] Activated!")


    def on_detections(self, depth_msg: Image, depth_info_msg: CameraInfo, detections_msg: DetectionArray | Detection2DArray) -> None:
        '''Process and publish detected objects for 2D/rgb mode'''
        detections = self.process_detections(depth_msg, depth_info_msg, detections_msg)
        self.process_objects(detections, detections_msg.header.stamp)

    def on_detections_pc(self, pc_msg: PointCloud2, depth_info_msg: CameraInfo, detections_msg) -> None:
        '''If using LiDAR, you must project the unorganized 3D Lidar points onto the 2D bounding box using camera_info intrinsics.'''
        self.get_logger().warn("PointCloud2 to 2D Bounding Box conversion requires PCL projection logic")
        pass

    def on_detections_3d(self, detections_3d_msg: DetectionArray | Detection3DArray) -> None:
        '''Process and publish detected objects for 3D/spatial mode'''
        detections_3d = self.process_detections_3d(detections_3d_msg)
        self.process_objects(detections_3d, detections_3d_msg.header.stamp)


    def process_objects(self, objects: List[ObjectPoint], stamp: Time) -> None:
        '''Publish markers and add objects to detection list'''
        self.get_logger().debug(f'{objects}')
        # Markers
        if (self.use_markers):
            msg = MarkerArray()
            for i, obj in enumerate(objects):
                cube_marker = self.get_marker(i, obj[0], obj[1], stamp, self.map_frame)
                text_marker = self.get_text_marker(i + 1000, obj[0], obj[1], stamp, self.map_frame)

                msg.markers.append(cube_marker)
                msg.markers.append(text_marker)
            
            self.publisher.publish(msg)
        
        # Transforms
        for i, obj in enumerate(objects):
            self.detected_objects[obj[0]].append(obj[1])


    def publish_objects(self) -> None:
        '''Publish the current transform of all detected objects'''
        for label, points in self.detected_objects.items():
            if len(points) >= self.min_samples:
                clean_points = self.remove_outlier_pos(points)  # remove outliers from all the object points

                if len(clean_points) >= self.min_samples:       # ensure there is enough samples
                    self.get_logger().debug(f'Validating consistency of target {label}')
                    avg_pos = np.mean(clean_points, axis=0)   # Calculate the average position of the block
                    std_dev = np.std(clean_points, axis=0)    # Calculate the standard deviation of the block's position

                    if np.all(std_dev < self.max_std_dev):      # Check that the standard deviation is small enough to be considered a confirmed block
                        self.get_logger().debug(f'Confirmed target {label} consistent pos at {avg_pos}')
                        self.publish_tf(label, avg_pos, self.get_clock().now().to_msg())
                        self.get_logger().info(f"[{self.get_name()} Object {label} at ({avg_pos})")
                    else:
                        self.get_logger().debug(f'Target {label} is not consistent enough.')
                else:
                    self.get_logger().debug(f'{label} has not enough samples to confirm')


    def process_detections(self, depth_msg: Image, depth_info_msg: CameraInfo, detections_msg: DetectionArray | Detection2DArray) -> List[ObjectPoint]:
        '''Process detections into a list of points with their respective colour'''
        if not detections_msg.detections:
            return []
        self.get_logger().debug(f'Processing detections')
        new_detections = []

        # calculate positions from bounding boxes
        def bbox_to_map_pos(bbox: BBox, depth_image:np.ndarray, depth_info_msg: CameraInfo):
            position = self.convert_bb_to_point(depth_image, depth_info_msg, bbox)
            if position is not None:
                new_position = self.tf_to_map(position, detections_msg.header.stamp)
                if new_position is not None:
                    return new_position
                else:
                    self.get_logger().warn(f'tf_to_map failed')
            else:
                self.get_logger().warn(f'bb_to_point failed')
            return None

        depth_image = self.cv_bridge.imgmsg_to_cv2(depth_msg, desired_encoding='32FC1')
        if self.use_vision_msgs:
            for detection in detections_msg.detections:
                # Detection will be of type Detection2D from vision_msgs

                bbox = (float(detection.bbox.center.position.x), 
                        float(detection.bbox.center.position.y), 
                        float(detection.bbox.size_x), float(detection.bbox.size_y))
                position = bbox_to_map_pos(bbox, depth_image, depth_info_msg)
                if position is not None:
                    score = 0
                    for result in detection.results:
                        if float(result.hypothesis.score) > score:
                            label:str = IDS_LABEL[int(result.hypothesis.class_id)]
                    new_detections.append((label, position))
        else:
            for detection in detections_msg.detections:
                # Detection will be of type Detection from yolo_msgs
                bbox = (float(detection.bbox.center.position.x), float(detection.bbox.center.position.y), float(detection.bbox.size.x), float(detection.bbox.size.y))
                position = bbox_to_map_pos(bbox, depth_image, depth_info_msg)
                if position is not None:
                    label:str = detection.class_name
                    new_detections.append((label, position))

        return new_detections


    def process_detections_3d(self, detections_msg: DetectionArray | Detection3DArray) -> List[ObjectPoint]:
        '''Process 3d detections into a list of points with their respective colour'''
        if not detections_msg.detections:
            return []
        self.get_logger().debug(f'Processing 3d detections')
        new_detections = []

        def get_pose_point(pose: Pose) -> Point:
            return float(pose.position.x), float(pose.position.y), float(pose.position.z)

        if self.use_vision_msgs:
            # detections_msg will be of Detection3DArray type. (vision_msgs)
            detection: Detection3D
            for detection in detections_msg.detections:
                hypothesis: ObjectHypothesisWithPose
                for hypothesis in detection.results:
                    position = get_pose_point(hypothesis.pose.pose)
                    map_position = self.tf_to_map(position, detections_msg.header.stamp)
                    if map_position is not None:
                        label:str = IDS_LABEL[int(hypothesis.hypothesis.class_id)]
                        new_detections.append((label, map_position))
        
        else:
            # NOTE: Currently this doesn't work with yolo_ros for some reason so just use 2D mode for sim, this node copies the bbox to point code anyway
            # detections_msg will be of DetectionArray type and have BoundingBox3D defined for each detection. (yolo_msgs)
            for detection in detections_msg.detections:
                detection: Detection
                position = get_pose_point(detection.bbox3d.center)
                map_position = self.tf_to_map(position, detections_msg.header.stamp)
                if map_position is not None:
                    label = detection.class_name
                    new_detections.append((label, map_position))
        
        return new_detections


    def convert_bb_to_point(self, depth_image: np.ndarray, depth_info: CameraInfo, bbox: BBox) -> Point | None:
        ''' Converts the bounding box center to a point relative to the image frame
            Modified from convert_bb_to_3d in https://github.com/mgonzs13/yolo_ros/blob/main/yolo_ros/yolo_ros/detect_3d_node.py
        '''
        self.get_logger().debug(f'Calculating obj world position relative to image frame')

        def resize_point(point:float, axis:int):
            """resize the point from rgb to depth resolution to account for differences"""
            return point * self.scale_factor[axis]

        center_x, center_y, size_x, size_y = [int(resize_point(x, i%2)) for i, x in enumerate(bbox)]
        
        # crop depth image by the 2d BB
        u_min = max(center_x - size_x // 2, 0)
        u_max = min(center_x + size_x // 2, depth_image.shape[1] - 1)
        v_min = max(center_y - size_y // 2, 0)
        v_max = min(center_y + size_y // 2, depth_image.shape[0] - 1)

        roi = depth_image[v_min:v_max, u_min:u_max]

        roi = roi / self.depth_image_units_divisor  # convert to meters
        if not np.any(roi):
            self.get_logger().warn(f"roi issue")
            return None

        # find the z coordinate on the 3D BB
        try:
            bb_center_z_coord = (
                depth_image[int(center_y)][int(center_x)] / self.depth_image_units_divisor
            )
        except IndexError: # handle occassional error by dropping it (bad fix lol)
            self.get_logger().warn("depth image index issue")
            return None

        z_diff = np.abs(roi - bb_center_z_coord)
        mask_z = z_diff <= self.maximum_detection_threshold
        if not np.any(mask_z):
            self.get_logger().warn("difference between center of object and average z value in roi not high enough")
            return None

        roi = roi[mask_z]
        z_min, z_max = np.min(roi), np.max(roi)
        z = (z_max + z_min) / 2

        if z == 0:
            self.get_logger().warn("z value fail") # idk what this checks for
            return None

        # project from image to world space
        k = depth_info.k
        px, py, fx, fy = k[2], k[5], k[0], k[4]
        x = z * (center_x - px) / fx
        y = z * (center_y - py) / fy
        #w = z * (size_x / fx)
        #h = z * (size_y / fy)
        #d = float(z_max - z_min)

        return (x, y, z)

    def quaternion_to_rotation_matrix(self, q: Tuple[float, float, float, float]) -> np.array:
        '''Convert a quaternion (x, y, z, w) into a 3x3 rotation matrix.'''
        x, y, z, w = q
        return np.array([
            [1 - 2 * (y ** 2 + z ** 2), 2 * (x * y - z * w), 2 * (x * z + y * w)],
            [2 * (x * y + z * w), 1 - 2 * (x ** 2 + z ** 2), 2 * (y * z - x * w)],
            [2 * (x * z - y * w), 2 * (y * z + x * w), 1 - 2 * (x ** 2 + y ** 2)]
        ])

    def tf_to_map(self, camera_to_obj: Point, stamp: Time) -> Point | None:
        '''Calculates the position tf from map to obj'''
        self.get_logger().debug(f'Calculating map to obj tf')
        point_stamped = PointStamped()
        point_stamped.header.frame_id = self.camera_frame
        point_stamped.header.stamp = stamp
        point_stamped.point.x, point_stamped.point.y, point_stamped.point.z = camera_to_obj
        try:
            transform = self.tf_buffer.lookup_transform(self.map_frame, self.camera_frame, stamp)
            transformed_point = tf2_geometry_msgs.do_transform_point(point_stamped, transform)
        except Exception as e:
            self.get_logger().warn(f'Error in calculating map to obj tf {e}')
            return None
        return (transformed_point.point.x, transformed_point.point.y, transformed_point.point.z)


    def get_marker(self, id:int, label:str, point: Point, stamp: Time, frame:str) -> Marker:
        '''Returns a marker derived from the detection'''
        marker = Marker()
        marker.pose.position.x, marker.pose.position.y, marker.pose.position.z = point

        # Fix projecting objects below the ground
        if marker.pose.position.z < 0:
            marker.pose.position.z = 0

        marker.pose.orientation.x, marker.pose.orientation.y, marker.pose.orientation.z, marker.pose.orientation.w = DEFAULT_QUATERNION

        marker.type = Marker.CUBE
        marker.scale.x = self.marker_size
        marker.scale.y = self.marker_size
        marker.scale.z = self.marker_size
        marker.color.r = LABELS[label][0]
        marker.color.g = LABELS[label][1]
        marker.color.b = LABELS[label][2]
        marker.color.a = 1.0

        marker.lifetime = Duration(seconds=self.marker_duration).to_msg()
        marker.ns = self.marker_ns
        marker.id = id

        marker.header.stamp = stamp
        marker.header.frame_id = frame

        return marker

    def get_text_marker(self, id:int, label:str, point: Point, stamp: Time, frame:str) -> Marker:
        '''Returns a text marker showing the label and coordinates'''
        marker = Marker()
        marker.header.stamp = stamp
        marker.header.frame_id = frame
        marker.ns = self.marker_ns + "_text"
        marker.id = id
        
        # Use the text view facing type so it always looks at the camera in RViz
        marker.type = Marker.TEXT_VIEW_FACING
        marker.action = Marker.ADD
        
        # Position the text slightly above the bounding box cube
        marker.pose.position.x, marker.pose.position.y, marker.pose.position.z = point
        if marker.pose.position.z < 0:
            marker.pose.position.z = 0
        marker.pose.position.z += self.marker_size + 0.1 
        
        marker.pose.orientation.x, marker.pose.orientation.y, marker.pose.orientation.z, marker.pose.orientation.w = DEFAULT_QUATERNION
        
        marker.scale.z = 0.15 
        
        # text color
        marker.color.r = 1.0
        marker.color.g = 1.0
        marker.color.b = 1.0
        marker.color.a = 1.0
        
        # Format the text to show the label and the X, Y, Z coordinates rounded to 2 decimals
        marker.text = f"{label}\n({point[0]:.2f}, {point[1]:.2f}, {point[2]:.2f})"
        
        marker.lifetime = Duration(seconds=self.marker_duration).to_msg()
        
        return marker

    def remove_outlier_pos(self, pos_vals: List[T]) -> List[T]:
        '''
        Removes any outlier positions from the list of positions. An outlier is defined as a position that is
        more than 3 standard deviations away from the mean.
        '''
        mean = np.mean(pos_vals, axis=0)
        std_dev = np.std(pos_vals, axis=0)

        return [pos for pos in pos_vals if np.all(np.abs(pos - mean) < 2 * std_dev)]

    def publish_tf(self, label:str, position: Point, stamp: Time) -> None:
        '''Publish the transform of a confirmed object'''
        tfs = TransformStamped()
        tfs.header.stamp = stamp
        tfs.header.frame_id = self.map_frame
        tfs.child_frame_id = label + '_obj'
        tfs.transform.translation.x,  tfs.transform.translation.y, tfs.transform.translation.z = position

        # Fix projecting objects below the ground
        if tfs.transform.translation.z < 0:
            tfs.transform.translation.z = 0

        tfs.transform.rotation.x, tfs.transform.rotation.y, tfs.transform.rotation.z, tfs.transform.rotation.w = DEFAULT_QUATERNION

        self.transform_broadcaster.sendTransform(tfs)



def main():
    rclpy.init()
    node = ObjectLocaliser()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()