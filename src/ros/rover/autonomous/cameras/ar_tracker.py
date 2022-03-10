__package__ = "autonomous"
import cv2
import cv2.aruco as ar
from config.ros_config import main_frame
from rclpy.node import Node
import rclpy
from core.msg import AlvarMarker
from config.ros_config import ar_track_topic
import os

class ArTracker(Node):
    def __init__(self):
        super().__init__("ar_tracker")
        self.publisher = self.create_publisher(AlvarMarker, ar_track_topic, 10)

    def __call__(self, img):
        msg = self.find_ar_tag(img)
        if msg:
            self.publisher.publish(msg)

    def find_ar_tag(self, img, markerSize=6, totalMarkers=250, draw=False):
        """
        Returns an AlvarMarker message or None
        """
        imgGray = cv2.cvtColor(img[:, :, [2, 1, 0]], cv2.COLOR_BGR2GRAY)
        arDict = ar.Dictionary_get(ar.DICT_5X5_250)
        arParam = ar.DetectorParameters_create()
        bboxs, ids, rejected = ar.detectMarkers(imgGray, arDict, parameters=arParam)
        # print("AR TAG DETECTED WITH ID")
        print("ids: " + str(ids))
        print(os.getcwd())
        camera_calibration_parameters_filename = "cameras/calibration_chessboard.yaml"
        aruco_marker_side_length = 0.1
        cv_file = cv2.FileStorage(camera_calibration_parameters_filename, cv2.FILE_STORAGE_READ)
        mtx = cv_file.getNode('K').mat()
        dst = cv_file.getNode('D').mat()
        cv_file.release()
        print("looking for tag")
        if ids is not None:
            print("drawing")
            ar.drawDetectedMarkers(img, bboxs)
            print("getting")
            rot_mat, trans_mat = ar.estimatePoseSingleMarkers(bboxs, aruco_marker_side_length, mtx, dst)
            print("found tag")
            for i, _id in enumerate(ids):
                
                print("___________________ AR TAGS ________________ " )
                print(i)
                print(_id)

                x = trans_mat[i][0][0]
                y = trans_mat[i][0][1]
                z = trans_mat[i][0][2]
                msg = AlvarMarker()
                msg.header.stamp = self.get_clock().now().to_msg()
                msg.header.frame_id = main_frame

                msg.pose.pose.position.x = x
                msg.pose.pose.position.y = y
                msg.pose.pose.position.z = z
                msg.id = int(_id[0])
                
                print("found tag: " + str(_id[0]))
                
                if msg.id == 15:
                    return msg

        else:
            print("no tag recognized")
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
