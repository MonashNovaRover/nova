#!/usr/bin/env python
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

ROS node that subscribes to ArUco detections and
    publishes each marker's pose as a TF frame.
Publishes a center frame as the centroid of all
    detected markers, with orientation averaged
    from marker orientations.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: usbc_localiser
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	auto_typing
AUTHOR(S):  Binuda Kalugalage
CREATION:	28/05/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

from geometry_msgs.msg import TransformStamped
from aruco_opencv_msgs.msg import ArucoDetection

import rclpy
from rclpy.node import Node
from rclpy.qos import qos_profile_sensor_data

from tf2_ros import TransformBroadcaster

import numpy as np
from scipy.spatial.transform import Rotation as R

ARUCO_TOPIC = '/aruco_detections'


class UsbcLocaliser(Node):
    def __init__(self):
        super().__init__('usbc_localiser')

        self.aruco_topic = self.declare_parameter(
            'aruco_topic', ARUCO_TOPIC
        ).get_parameter_value().string_value

        self.frame_prefix = self.declare_parameter(
            'frame_prefix', 'aruco_marker_'
        ).get_parameter_value().string_value

        self.center_frame = self.declare_parameter(
            'center_frame', 'usbc_frame'
        ).get_parameter_value().string_value

        # Which marker IDs to use for computing the center frame
        self.marker_ids = list(self.declare_parameter(
            'marker_ids', [4, 1, 3, 2]
        ).get_parameter_value().integer_array_value)

        # Minimum markers needed to publish center frame
        self.min_markers = self.declare_parameter(
            'min_markers', 2
        ).get_parameter_value().integer_value

        self.transform_broadcaster = TransformBroadcaster(self)

        self.aruco_sub = self.create_subscription(
            ArucoDetection,
            self.aruco_topic,
            self.aruco_callback,
            qos_profile=qos_profile_sensor_data,
        )

        self.get_logger().info(
            f"USB-C localiser started, subscribing to {self.aruco_topic}, "
            f"marker_ids={self.marker_ids}, center_frame={self.center_frame}"
        )

    def aruco_callback(self, detection: ArucoDetection) -> None:
        """Publish a TF frame for each detected marker, plus the center frame."""
        transforms = []

        for marker in detection.markers:
            tfs = TransformStamped()
            tfs.header = detection.header
            tfs.child_frame_id = f"{self.frame_prefix}{marker.marker_id}"

            tfs.transform.translation.x = marker.pose.position.x
            tfs.transform.translation.y = marker.pose.position.y
            tfs.transform.translation.z = marker.pose.position.z

            tfs.transform.rotation.x = marker.pose.orientation.x
            tfs.transform.rotation.y = marker.pose.orientation.y
            tfs.transform.rotation.z = marker.pose.orientation.z
            tfs.transform.rotation.w = marker.pose.orientation.w

            transforms.append(tfs)

        center_tf = self.estimate_center(detection)
        if center_tf is not None:
            transforms.append(center_tf)

        if transforms:
            self.transform_broadcaster.sendTransform(transforms)

    def estimate_center(self, detection: ArucoDetection) -> TransformStamped | None:
        """Compute center frame as centroid of detected marker positions,
        with orientation averaged from marker orientations."""
        positions = []
        quaternions = []

        marker_lookup = {m.marker_id: m for m in detection.markers}
        for mid in self.marker_ids:
            if mid not in marker_lookup:
                continue
            m = marker_lookup[mid]
            positions.append([m.pose.position.x, m.pose.position.y, m.pose.position.z])
            quaternions.append([m.pose.orientation.x, m.pose.orientation.y, m.pose.orientation.z, m.pose.orientation.w])

        if len(positions) < self.min_markers:
            return None

        centroid = np.mean(positions, axis=0)
        avg_quat = self.average_quaternions(np.array(quaternions))

        tfs = TransformStamped()
        tfs.header = detection.header
        tfs.child_frame_id = self.center_frame
        tfs.transform.translation.x = float(centroid[0])
        tfs.transform.translation.y = float(centroid[1])
        tfs.transform.translation.z = float(centroid[2])
        tfs.transform.rotation.x = float(avg_quat[0])
        tfs.transform.rotation.y = float(avg_quat[1])
        tfs.transform.rotation.z = float(avg_quat[2])
        tfs.transform.rotation.w = float(avg_quat[3])

        return tfs

    def average_quaternions(self, quats: np.ndarray) -> np.ndarray:
        """Average quaternions using the eigenvector method.
        quats: (N, 4) array of [x, y, z, w] quaternions.
        Returns averaged [x, y, z, w] quaternion."""
        # Flip quaternions to same hemisphere to avoid cancellation
        for i in range(1, len(quats)):
            if np.dot(quats[i], quats[0]) < 0:
                quats[i] *= -1

        # Largest eigenvector of Q^T Q gives the average
        M = quats.T @ quats
        eigenvalues, eigenvectors = np.linalg.eigh(M)
        avg = eigenvectors[:, np.argmax(eigenvalues)]

        # Ensure w > 0 for consistency
        if avg[3] < 0:
            avg *= -1

        return avg / np.linalg.norm(avg)


def main():
    rclpy.init()
    node = UsbcLocaliser()
    rclpy.spin(node)
    rclpy.shutdown()


if __name__ == "__main__":
    main()
