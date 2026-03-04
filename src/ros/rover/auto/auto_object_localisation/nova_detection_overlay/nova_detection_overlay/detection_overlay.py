#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image
from vision_msgs.msg import Detection2DArray, Detection3DArray
import cv2
from cv_bridge import CvBridge
import numpy as np
from std_msgs.msg import Header
from rclpy.qos import qos_profile_sensor_data

class Detection2DOverlay(Node):

    def __init__(self):
        super().__init__('detection_overlay')

        image_topic = self.declare_parameter('image_topic', '/camera/color/image_raw').value
        det_2d_topic = self.declare_parameter('detections_2d_topic', '/detections/2d').value
        det_3d_topic = self.declare_parameter('detections_3d_topic', '/detections/3d').value
        self.frame_id = self.declare_parameter('frame_id', 'camera_color_optical_frame').value

        self.preview_sub = self.create_subscription(
            Image, image_topic, self.preview_callback, qos_profile=qos_profile_sensor_data)
        
        self.det_sub = self.create_subscription(
            Detection2DArray, det_2d_topic, self.det_callback, qos_profile=qos_profile_sensor_data)
            
        self.det3d_sub = self.create_subscription(
            Detection3DArray, det_3d_topic, self.det3d_callback, qos_profile=qos_profile_sensor_data)
            
        self.overlay_pub = self.create_publisher(Image, 'overlay', 10)
        self.preview = None

    def preview_callback(self, preview):
        self.preview = preview

    def det_callback(self, detections):
        self.publish_overlay(detections, True)

    def det3d_callback(self, detections):
        self.publish_overlay(detections, False)

    def publish_overlay(self, detections, is_2d):
        preview_mat = self.msg_to_mat(self.get_logger(), self.preview, 'bgr8')

        blue = (255, 0, 0)

        if preview_mat is None:
            return
        
        for detection in detections.detections:
            if is_2d:
                x1 = detection.bbox.center.position.x - detections.detections[0].bbox.size_x / 2.0
                x2 = detection.bbox.center.position.x + detections.detections[0].bbox.size_x / 2.0
                y1 = detection.bbox.center.position.y - detections.detections[0].bbox.size_y / 2.0
                y2 = detection.bbox.center.position.y + detections.detections[0].bbox.size_y / 2.0
            else:
                x1 = detection.bbox.center.position.x - detections.detections[0].bbox.size.x / 2.0
                x2 = detection.bbox.center.position.x + detections.detections[0].bbox.size.x / 2.0
                y1 = detection.bbox.center.position.y - detections.detections[0].bbox.size.y / 2.0
                y2 = detection.bbox.center.position.y + detections.detections[0].bbox.size.y / 2.0

            label_str = str(detection.results[0].hypothesis.class_id)
            confidence = detection.results[0].hypothesis.score
            self.add_text_to_frame(preview_mat, label_str, x1 + 10, y1 + 20)
            conf_str = '{:.2f}'.format(confidence * 100)
            self.add_text_to_frame(preview_mat, conf_str, x1 + 10, y1 + 40)
            cv2.rectangle(preview_mat, (int(x1), int(y1)), (int(x2), int(y2)), blue)

        out_msg = CvBridge().cv2_to_imgmsg(preview_mat, 'bgr8')
        out_msg.header.frame_id = 'oak_rgb_camera_optical_frame'

        self.overlay_pub.publish(out_msg)

    def msg_to_mat(self, logger, img, encoding):
        mat = None
        try:
            mat = CvBridge().imgmsg_to_cv2(img, encoding)
        except Exception as e:
            logger.error(str(e))
        return mat

    def add_text_to_frame(self, frame, text, x, y):
        white = (255, 255, 255)
        black = (0, 0, 0)
        
        x = int(x)
        y = int(y)
        cv2.putText(frame, text, (x, y), cv2.FONT_HERSHEY_DUPLEX, 0.5, white, 3)
        cv2.putText(frame, text, (x, y), cv2.FONT_HERSHEY_DUPLEX, 0.5, black)

def main(args=None):
    rclpy.init(args=args)
    node = Detection2DOverlay()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()

