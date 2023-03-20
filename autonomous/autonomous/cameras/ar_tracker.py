__package__ = "autonomous"
import cv2
from cv2 import aruco as ar
from rclpy.node import Node
import rclpy
try:
    from pyrealsense2 import intrinsics as Intrinsics
except Exception:
    from pyrealsense2.pyrealsense2 import intrinsics as Intrinsics

from geometry_msgs.msg import PoseStamped, TransformStamped
from core.msg import AlvarMarker, AlvarMarkers
from tf2_ros import TransformBroadcaster
import numpy as np
import logging


class ArTracker(Node):
    def __init__(self, intrinsics: Intrinsics, depth_cam_frame_id="d435_1"):
        super().__init__("ar_tracker")
        self.get_logger().set_level(logging.INFO)

        self.frame_id = depth_cam_frame_id

        self.marker_width_m = 0.15
        self.ar_pose_pub = self.create_publisher(AlvarMarkers, "~/tags", 10)

        self.arDict = ar.Dictionary_get(ar.DICT_4X4_250)
        self.arParam = ar.DetectorParameters_create()

        # getting focal length and focal centre from camera intrinsics
        fx, fy = intrinsics.fx, intrinsics.fy
        cx, cy = intrinsics.ppx, intrinsics.ppy

        self.get_logger().debug(f"Intrinsics: {intrinsics}")

        self.intrinsics = np.array([[fx, 0, cx], [0, fy, cy], [0, 0, 1]])
        # realsense 400 series depth cameras do image processing to remove distortion
        self.distortion = np.array([0, 0, 0, 0, 0])

        self.temp_tags = dict()
        self.tags = dict()

    def get_pose(self, r, t) -> PoseStamped:
        """
        r: rotation vector (3, 1)
        t: translation vector (3, 1)
        """
        self.get_logger().debug(f"received rotation vector: {r}")
        self.get_logger().debug(f"received translation vector: {t}")
        pose = PoseStamped()

        pose.header.stamp = self.get_clock().now().to_msg()
        pose.header.frame_id = self.frame_id

        # extracting position
        pose.pose.position.x = t[0]
        pose.pose.position.y = t[1]
        pose.pose.position.z = t[2]

        # convert rotation vector to quaternion
        theta = np.linalg.norm(r) # magnitude of vector gives rotation
        unit_rvec = r / theta # normalise vector

        # Calculating quaternion (https://en.wikipedia.org/wiki/Quaternion) 
        pose.pose.orientation.w = np.cos(theta/2)
        pose.pose.orientation.x = unit_rvec[0] * np.sin(theta/2)
        pose.pose.orientation.y = unit_rvec[1] * np.sin(theta/2)
        pose.pose.orientation.z = unit_rvec[2] * np.sin(theta/2)

        # converting rotation matrix to quaternion
        self.get_logger().debug(f"AR tag pose: {pose.pose}")

        return pose

    def find_ar_tags(self, img):
        """
        Returns an AlvarMarker message or None
        """
        bboxs, ids, _ = ar.detectMarkers(img, self.arDict, parameters=self.arParam)
        markers = AlvarMarkers()

        if ids is not None:
            # projecting 2d camera coordinates into space
            pose = ar.estimatePoseSingleMarkers(bboxs, self.marker_width_m, self.intrinsics, self.distortion)
            rot_mats, trans_mats = pose[0], pose[1]
            for _id, rot_mat, trans_mat in zip(ids, rot_mats, trans_mats):
                print(rot_mat, trans_mat)
                pose = self.get_pose(rot_mat[0], trans_mat[0])
                AR_tag = AlvarMarker(tag_id=int(_id), pose=pose)
                markers.markers.append(AR_tag)
        self.ar_pose_pub.publish(markers)

def main():
    rclpy.init()
    cap = cv2.VideoCapture(0)

    tracker = ArTracker()

    while True:
        success, img = cap.read()
        tracker.find_ar_tag(img)
        cv2.imshow("Image", img)
        cv2.waitKey(1)

if __name__ == "__main__":
    main()
