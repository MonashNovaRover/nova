__package__ = "autonomous"
import cv2
import cv2.aruco as ar
from config.ros_config import main_frame
from rclpy.node import Node
import rclpy
from core.msg import AlvarMarker
from config.ros_config import ar_track_topic
from config.runtime_params import max_fov_angle
import os
from nav_msgs.msg import Odometry
import numpy as np

save_pt: bool = False

class ArTracker(Node):
    def __init__(self):
        super().__init__("ar_tracker")
        self.publisher = self.create_publisher(AlvarMarker, ar_track_topic, 10)
        self.odom_publisher = self.create_publisher(Odometry, "autonomous/ar_tag/odometry", 10)
        self.previous_10_tags = []

    def store_ar_coords(self, msg):
        if len(self.previous_10_tags) < 10:
            tag = np.array([msg.pose.pose.position.x, msg.pose.pose.position.y])
            self.previous_10_tags.append(tag)
        else: 
            coord = input("enter real artag pose as tuple: ")
            true_coord = np.array([float(coord.split()[0]), float(coord.split()[1])])
            
            approx_coords = np.array(self.previous_10_tags)
            approx_coord = np.sum(approx_coords, axis=0) / (len(approx_coords))
            print(f"true coordinate x = {true_coord[0]}, y = {true_coord[1]}")
            print(f"approx coordinate x = {approx_coord[0]}, y = {approx_coord[1]}")

            try:
                approx, true = np.load("/home/nvidia/nova_ws/src/autonomous/autonomous/cameras/approx.npy").tolist(), np.load("/home/nvidia/nova_ws/src/autonomous/autonomous/cameras/true.npy").tolist()
                approx.append(approx_coord)
                true.append(true_coord)
                approx = np.array(approx)
                true = np.array(true)
                print(approx)
                print(true)
                np.save("/home/nvidia/nova_ws/src/autonomous/autonomous/cameras/approx.npy", approx)
                np.save("/home/nvidia/nova_ws/src/autonomous/autonomous/cameras/true.npy", true)
            except Exception as e:
                print(approx_coord)
                print(true_coord)
                approx = np.array([approx_coord])
                true = np.array([true_coord])
                np.save("/home/nvidia/nova_ws/src/autonomous/autonomous/cameras/approx.npy", np.array(approx).reshape(1, 2))
                np.save("/home/nvidia/nova_ws/src/autonomous/autonomous/cameras/true.npy", np.array(true).reshape(1, 2))
            rclpy.shutdown()

    def __call__(self, img):
        if save_pt:
            print("saving point")
            msg = self.find_ar_tag_no_transform(img)
        else:
            msg = self.find_ar_tag(img)
        if msg:
            if save_pt: self.store_ar_coords(msg)            
            #self.publisher.publish(msg)
            if msg.pose.pose.position.x == 0: return
            odom = Odometry()
            odom.header.frame_id = main_frame
            odom.header.stamp = self.get_clock().now().to_msg()
            odom.pose.pose.position.x = msg.pose.pose.position.x
            odom.pose.pose.position.y = msg.pose.pose.position.y
            odom.pose.pose.position.z = msg.pose.pose.position.z
            if True:#abs(np.arctan2(odom.pose.pose.position.y, odom.pose.pose.position.x)) < max_fov_angle:
                #print(f"x = {odom.pose.pose.position.x}, y = {odom.pose.pose.position.y}")
                self.publisher.publish(msg)
                self.odom_publisher.publish(odom)

    def find_ar_tag_no_transform(self, img, markerSize=6, totalMarkers=250, draw=False):
        """
        Returns an AlvarMarker message or None
        """
        imgGray = cv2.cvtColor(img[:, :, [2, 1, 0]], cv2.COLOR_BGR2GRAY)
        arDict = ar.Dictionary_get(ar.DICT_4X4_250)
        arParam = ar.DetectorParameters_create()
        bboxs, ids, rejected = ar.detectMarkers(imgGray, arDict, parameters=arParam)
        camera_calibration_parameters_filename = "cameras/calib_chessboard.yaml"
        aruco_marker_side_length = 0.2
        cv_file = cv2.FileStorage(camera_calibration_parameters_filename, cv2.FILE_STORAGE_READ)
        mtx = cv_file.getNode('K').mat()
        dst = cv_file.getNode('D').mat()
        #undistorted = cv2.undistort(imgGray, mtx, dst)
        #cv2.imshow("pic", undistorted)
        #cv2.waitKey(0)
        cv_file.release()
        if ids is not None:
            ar.drawDetectedMarkers(img, bboxs)
            
            if cv2.__version__ == '3.2.0':
                rot_mat, trans_mat, = ar.estimatePoseSingleMarkers(bboxs, aruco_marker_side_length, mtx, dst)
            else:
                rot_mat, trans_mat, _ = ar.estimatePoseSingleMarkers(bboxs, aruco_marker_side_length, mtx, dst)

            for i, _id in enumerate(ids):
                x = trans_mat[i][0][2]
                y = -trans_mat[i][0][0]
                z = -trans_mat[i][0][1]

                print((x, y, z))
                # transform = np.load("cameras/ar_2d_calibration.npy")
                # A = transform[:2, :].T
                # t = transform[-1, :].T

                #pose = np.array([x, y]) 
                pose = np.array([x, y])
                pose.reshape(2)
                print(pose)

                msg = AlvarMarker()
                msg.header.stamp = self.get_clock().now().to_msg()
                msg.header.frame_id = main_frame
                msg.pose.pose.position.x = pose[0]
                msg.pose.pose.position.y = pose[1]
                msg.pose.pose.position.z = z
                msg.id = int(_id[0])
                #self.get_logger().info(f"detected ar tag with id {msg.id}")
                return msg
        else:
            return None
        return None


    def find_ar_tag(self, img, markerSize=6, totalMarkers=250, draw=False):
        """
        Returns an AlvarMarker message or None
        """
        imgGray = cv2.cvtColor(img[:, :, [2, 1, 0]], cv2.COLOR_BGR2GRAY)
        arDict = ar.Dictionary_get(ar.DICT_4X4_250)
        arParam = ar.DetectorParameters_create()
        bboxs, ids, rejected = ar.detectMarkers(imgGray, arDict, parameters=arParam)
        camera_calibration_parameters_filename = "cameras/calib_chessboard.yaml"
        aruco_marker_side_length = 0.2
        cv_file = cv2.FileStorage(camera_calibration_parameters_filename, cv2.FILE_STORAGE_READ)
        mtx = cv_file.getNode('K').mat()
        dst = cv_file.getNode('D').mat()
        #undistorted = cv2.undistort(imgGray, mtx, dst)
        #cv2.imshow("pic", undistorted)
        #cv2.waitKey(0)
        cv_file.release()
        if ids is not None:
            ar.drawDetectedMarkers(img, bboxs)
            
            if cv2.__version__ == '3.2.0':
                rot_mat, trans_mat, = ar.estimatePoseSingleMarkers(bboxs, aruco_marker_side_length, mtx, dst)
            else:
                rot_mat, trans_mat, _ = ar.estimatePoseSingleMarkers(bboxs, aruco_marker_side_length, mtx, dst)

            for i, _id in enumerate(ids):
                x = trans_mat[i][0][2]
                y = -trans_mat[i][0][0]
                z = -trans_mat[i][0][1]

                print((x, y, z))
                transform = np.load("cameras/ar_2d_calibration.npy")
                A = transform[:2, :].T
                t = transform[-1, :].T

                #pose = np.array([x, y]) 
                pose = np.matmul(A, np.array([x, y]).T) + t
                pose.reshape(2)
                print(pose)

                msg = AlvarMarker()
                msg.header.stamp = self.get_clock().now().to_msg()
                msg.header.frame_id = main_frame
                msg.pose.pose.position.x = pose[0]
                msg.pose.pose.position.y = pose[1]
                msg.pose.pose.position.z = z
                msg.id = int(_id[0])
                #self.get_logger().info(f"detected ar tag with id {msg.id}")
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

