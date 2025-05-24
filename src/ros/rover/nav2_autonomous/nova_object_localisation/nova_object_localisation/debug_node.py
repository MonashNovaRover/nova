#!/usr/bin/env python3

# Copyright (C) 2023 Miguel Ángel González Santamarta

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: debug_node
TOPICS:
  - subscriber: /oak/nn/detections  [vision_msgs/msg/Detection2DArray]
  - publisher: /yolo/debug_image    [sensor_msgs/msg/Image]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_object_localisation
AUTHOR(S):	Victor Bartlinski
CREATION:	24/05/2025
EDITED:		24/05/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''

import cv2
import random
import numpy as np
from typing import Tuple

import rclpy
from rclpy.duration import Duration
from rclpy.qos import QoSProfile
from rclpy.qos import QoSHistoryPolicy
from rclpy.qos import QoSDurabilityPolicy
from rclpy.qos import QoSReliabilityPolicy
from rclpy.lifecycle import LifecycleNode
from rclpy.lifecycle import TransitionCallbackReturn
from rclpy.lifecycle import LifecycleState

import message_filters
from cv_bridge import CvBridge
from ultralytics.utils.plotting import Annotator, colors

from sensor_msgs.msg import Image
from visualization_msgs.msg import Marker
from visualization_msgs.msg import MarkerArray
from yolo_msgs.msg import BoundingBox2D, KeyPoint2D, KeyPoint2DArray, KeyPoint3D, Detection, DetectionArray
from vision_msgs.msg import Detection2D, Detection2DArray
from tf2_ros import Buffer, TransformListener


# LABELS = {'red':[1.0,0.0,0.0], 'green':[0.0,1.0,0.0], 'blue':[0.0,0.0,1.0], 'white':[1.0,1.0,1.0]} # ARCh 2025
LABELS = {'bottle':[0.0,0.0,1.0], 'mallet':[1.0,0.0,0.0]} # URC 2025
DEFAULT_QUATERNION = [0.0, 0.0, 0.0, 1.0]

# Object ids:
# Note: This has the same order as mappings in the generated .json file
# IDS_LABEL = { 0: 'blue', 1: 'green', 2: 'red', 3: 'white'} # ARCh 2025
IDS_LABEL = { 0: 'bottle', 1: 'mallet'} # URC 2025


class DebugNode(LifecycleNode):

    def __init__(self) -> None:
        super().__init__("debug_node")

        self._class_to_color = {}
        self.cv_bridge = CvBridge()

        # params
        self.declare_parameter("image_reliability", QoSReliabilityPolicy.BEST_EFFORT)

        self.tf_buffer = Buffer()
        self.tf_listener = TransformListener(self.tf_buffer, self)

    def on_configure(self, state: LifecycleState) -> TransitionCallbackReturn:
        self.get_logger().info(f"[{self.get_name()}] Configuring...")

        self.image_qos_profile = QoSProfile(
            reliability=self.get_parameter("image_reliability")
            .get_parameter_value()
            .integer_value,
            history=QoSHistoryPolicy.KEEP_LAST,
            durability=QoSDurabilityPolicy.VOLATILE,
            depth=1,
        )

        # pubs
        self._dbg_pub = self.create_publisher(Image, "dbg_image", 10)

        super().on_configure(state)
        self.get_logger().info(f"[{self.get_name()}] Configured")

        return TransitionCallbackReturn.SUCCESS

    def on_activate(self, state: LifecycleState) -> TransitionCallbackReturn:
        self.get_logger().info(f"[{self.get_name()}] Activating...")

        # subs
        self.image_sub = message_filters.Subscriber(
            self, Image, "image_raw", qos_profile=self.image_qos_profile
        )
        self.detections_sub = message_filters.Subscriber(
            self, Detection2DArray, "detections", qos_profile=10
        )

        self._synchronizer = message_filters.ApproximateTimeSynchronizer(
            (self.image_sub, self.detections_sub), 10, 0.5
        )
        self._synchronizer.registerCallback(self.detections_cb)

        super().on_activate(state)
        self.get_logger().info(f"[{self.get_name()}] Activated")

        return TransitionCallbackReturn.SUCCESS

    def on_deactivate(self, state: LifecycleState) -> TransitionCallbackReturn:
        self.get_logger().info(f"[{self.get_name()}] Deactivating...")

        self.destroy_subscription(self.image_sub.sub)
        self.destroy_subscription(self.detections_sub.sub)

        del self._synchronizer

        super().on_deactivate(state)
        self.get_logger().info(f"[{self.get_name()}] Deactivated")

        return TransitionCallbackReturn.SUCCESS

    def on_cleanup(self, state: LifecycleState) -> TransitionCallbackReturn:
        self.get_logger().info(f"[{self.get_name()}] Cleaning up...")

        self.destroy_publisher(self._dbg_pub)

        super().on_cleanup(state)
        self.get_logger().info(f"[{self.get_name()}] Cleaned up")

        return TransitionCallbackReturn.SUCCESS

    def on_shutdown(self, state: LifecycleState) -> TransitionCallbackReturn:
        self.get_logger().info(f"[{self.get_name()}] Shutting down...")
        super().on_cleanup(state)
        self.get_logger().info(f"[{self.get_name()}] Shutted down")
        return TransitionCallbackReturn.SUCCESS

    def draw_box(
        self,
        cv_image: np.ndarray,
        detection: Detection,
        color: Tuple[int],
    ) -> np.ndarray:

        # get detection info
        class_name = detection.class_name
        score = detection.score
        box_msg: BoundingBox2D = detection.bbox
        track_id = detection.id

        min_pt = (
            round(box_msg.center.position.x - box_msg.size.x / 2.0),
            round(box_msg.center.position.y - box_msg.size.y / 2.0),
        )
        max_pt = (
            round(box_msg.center.position.x + box_msg.size.x / 2.0),
            round(box_msg.center.position.y + box_msg.size.y / 2.0),
        )

        # define the four corners of the rectangle
        rect_pts = np.array(
            [
                [min_pt[0], min_pt[1]],
                [max_pt[0], min_pt[1]],
                [max_pt[0], max_pt[1]],
                [min_pt[0], max_pt[1]],
            ]
        )

        # # calculate the rotation matrix
        # rotation_matrix = cv2.getRotationMatrix2D(
        #     (box_msg.center.position.x, box_msg.center.position.y),
        #     -np.rad2deg(box_msg.center.theta),
        #     1.0,
        # )

        # # rotate the corners of the rectangle
        # rect_pts = np.int0(cv2.transform(np.array([rect_pts]), rotation_matrix)[0])

        # Draw the rotated rectangle
        for i in range(4):
            pt1 = tuple(rect_pts[i])
            pt2 = tuple(rect_pts[(i + 1) % 4])
            cv2.line(cv_image, pt1, pt2, color, 2)

        # write text
        label = f"{class_name}"
        label += f" ({track_id})" if track_id else ""
        label += " ({:.3f})".format(score)
        pos = (min_pt[0] + 5, min_pt[1] + 25)
        font = cv2.FONT_HERSHEY_SIMPLEX
        cv2.putText(cv_image, label, pos, font, 1, color, 1, cv2.LINE_AA)

        return cv_image

    def draw_keypoints(self, cv_image: np.ndarray, detection: Detection) -> np.ndarray:

        keypoints_msg = detection.keypoints

        ann = Annotator(cv_image)

        kp: KeyPoint2D
        for kp in keypoints_msg.data:
            color_k = (
                [int(x) for x in ann.kpt_color[kp.id - 1]]
                if len(keypoints_msg.data) == 17
                else colors(kp.id - 1)
            )

            cv2.circle(
                cv_image,
                (int(kp.point.x), int(kp.point.y)),
                5,
                color_k,
                -1,
                lineType=cv2.LINE_AA,
            )
            cv2.putText(
                cv_image,
                str(kp.id),
                (int(kp.point.x), int(kp.point.y)),
                cv2.FONT_HERSHEY_SIMPLEX,
                1,
                color_k,
                1,
                cv2.LINE_AA,
            )

        def get_pk_pose(kp_id: int) -> Tuple[int]:
            for kp in keypoints_msg.data:
                if kp.id == kp_id:
                    return (int(kp.point.x), int(kp.point.y))
            return None

        for i, sk in enumerate(ann.skeleton):
            kp1_pos = get_pk_pose(sk[0])
            kp2_pos = get_pk_pose(sk[1])

            if kp1_pos is not None and kp2_pos is not None:
                cv2.line(
                    cv_image,
                    kp1_pos,
                    kp2_pos,
                    [int(x) for x in ann.limb_color[i]],
                    thickness=2,
                    lineType=cv2.LINE_AA,
                )

        return cv_image

    def detections_cb(self, img_msg: Image, detection_msg: Detection2DArray) -> None:
        cv_image = self.cv_bridge.imgmsg_to_cv2(img_msg, desired_encoding="bgr8")

        try:
            transform = self.tf_buffer.lookup_transform('map', 'base_link', rclpy.time.Time(), rclpy.time.Duration(seconds=10.0))
        except Exception as e:
            self.get_logger().error(f' ❌ Transform was not available! {e}')

        detection_2d: Detection2D
        for detection_2d in detection_msg.detections:

            detection = Detection()
            detection.class_id = int(detection_2d.results[0].hypothesis.class_id)
            detection.class_name = IDS_LABEL[int(IDS_LABEL[int(detection_2d.results[0].hypothesis.class_id)])]
            detection.score = detection_2d.results[0].hypothesis.score
            detection.id = int(detection_2d.results[0].hypothesis.class_id)
            detection.bbox.center.position.x = detection_2d.bbox.center.position.x
            detection.bbox.center.position.y = detection_2d.bbox.center.position.y
            detection.bbox.center.theta = detection_2d.bbox.center.theta
            detection.bbox.size.x = detection_2d.bbox.size_x
            detection.bbox.size.y = detection_2d.bbox.size_y
            keypoint = KeyPoint2D()
            keypoint.id = int(detection_2d.results[0].hypothesis.class_id)
            keypoint.point.x = detection_2d.bbox.center.position.x
            keypoint.point.y = detection_2d.bbox.center.position.y
            keypoint.score  = detection_2d.results[0].hypothesis.score
            detection.keypoints.data = [keypoint]

            # random color
            class_name = detection.class_name

            if class_name not in self._class_to_color:
                r = random.randint(0, 255)
                g = random.randint(0, 255)
                b = random.randint(0, 255)
                self._class_to_color[class_name] = (r, g, b)

            color = self._class_to_color[class_name]

            cv_image = self.draw_box(cv_image, detection, color)
            cv_image = self.draw_keypoints(cv_image, detection)

        # publish dbg image
        self._dbg_pub.publish(
            self.cv_bridge.cv2_to_imgmsg(cv_image, encoding="bgr8", header=img_msg.header)
        )


def main():
    rclpy.init()
    node = DebugNode()
    node.trigger_configure()
    node.trigger_activate()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()