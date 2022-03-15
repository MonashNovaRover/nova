__package__ = "autonomous"
import cv2
import cv2.aruco as ar
from config.ros_config import main_frame
from rclpy.node import Node
import rclpy
from core.msg import AlvarMarker
from config.ros_config import ar_track_topic
import os
from nav_msgs.msg import Odometry
import numpy as np

class ArTracker(Node):
    def __init__(self):
        super().__init__("ar_tracker")
        self.publisher = self.create_publisher(AlvarMarker, ar_track_topic, 10)
        self.odom_publisher = self.create_publisher(Odometry, "autonomous/ar_tag/odometry", 10)

    def __call__(self, img):
        msg = self.find_ar_tag(img)
        if msg:
            self.publisher.publish(msg)
            if msg.pose.pose.position.x == 0: return
            odom = Odometry()
            odom.header.frame_id = main_frame
            odom.header.stamp = self.get_clock().now().to_msg()
            odom.pose.pose.position.x = msg.pose.pose.position.x
            odom.pose.pose.position.y = msg.pose.pose.position.y
            odom.pose.pose.position.z = msg.pose.pose.position.z
            self.publisher.publish(msg)
            self.odom_publisher.publish(odom)

    def find_ar_tag(self, img, markerSize=6, totalMarkers=250, draw=False):
        """
        Returns an AlvarMarker message or None
        """
        imgGray = cv2.cvtColor(img[:, :, [2, 1, 0]], cv2.COLOR_BGR2GRAY)
        arDict = ar.Dictionary_get(ar.DICT_5X5_250)
        arParam = ar.DetectorParameters_create()
        bboxs, ids, rejected = ar.detectMarkers(imgGray, arDict, parameters=arParam)
        camera_calibration_parameters_filename = "cameras/calib_chessboard.yaml"
        aruco_marker_side_length = 0.1
        cv_file = cv2.FileStorage(camera_calibration_parameters_filename, cv2.FILE_STORAGE_READ)
        mtx = cv_file.getNode('K').mat()
        dst = cv_file.getNode('D').mat()
        #undistorted = cv2.undistort(imgGray, mtx, dst)
        #cv2.imshow("pic", undistorted)
        #cv2.waitKey(0)
        cv_file.release()
        if ids is not None:
            ar.drawDetectedMarkers(img, bboxs)
            
            rot_mat, trans_mat, obs = ar.estimatePoseSingleMarkers(bboxs, aruco_marker_side_length, mtx, dst)
            #if rot_mat is not None:
            #    ar.drawAxis(img, mtx, dst, rot_mat, trans_mat, 0.05)
            #    cv2.imshow("pic", img)
            #    cv2.waitKey(0)
            for i, _id in enumerate(ids):
                x = trans_mat[i][0][2]
                y = -trans_mat[i][0][0]
                z = -trans_mat[i][0][1]
                msg = AlvarMarker()
                msg.header.stamp = self.get_clock().now().to_msg()
                msg.header.frame_id = main_frame
                arg = np.pi/6
                msg.pose.pose.position.x = x*np.cos(arg) - y*np.sin(arg) 
                msg.pose.pose.position.y = x*np.sin(arg) + y*np.cos(arg)
                msg.pose.pose.position.z = z
                msg.id = int(_id[0])
                return msg
        else:
            return None
        return None


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
