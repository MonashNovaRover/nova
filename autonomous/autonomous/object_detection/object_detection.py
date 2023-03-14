__package__ = "autonomous"
import cv2
import time
import numpy as np
from rclpy.node import Node
from visualization_msgs.msg import Marker, MarkerArray
from geometry_msgs.msg import Pose
from std_msgs.msg import ColorRGBA
import autonomous.object_detection.config_files as models
from ultralytics import YOLO
import pyrealsense2 as rs
import os
import logging

try:
    from pyrealsense2 import intrinsics as Intrinsics
except Exception:
    from pyrealsense2.pyrealsense2 import intrinsics as Intrinsics

class ObjectDetection(Node):
    def __init__(self, intrinsics: Intrinsics, frame_id: str = "d435_1", show: bool = False, confidence_threshold: float = 0.6):
        """
        Initialise object detection node
        :param intrinsics: intrinsics of the camera
        :param model_path: path to the model
        :param show: whether to show the image (used for testing)
        """
        super().__init__("object_detector")
        self.get_logger().set_level(logging.DEBUG)
        self.obj_pub = self.create_publisher(MarkerArray, f"~/markers", 10)
        # Get full path to model from the obj_detect package using os
        model_path = os.path.join(os.path.dirname(models.__file__), "10_mar_cubes_nano.pt")
        
        self.model = YOLO(model_path) 
        self.frame_id = frame_id
        self.get_logger().debug(f"Intrinsics: {intrinsics}")
        self.intr = intrinsics
        self.show = True
        self.confidence_threshold = confidence_threshold

    def get_marker(self, point, c: tuple) -> None:
        """
        :params: c is color tuple (r,g,b) between 0 and 1
        """
        msg = Marker()
        pose = Pose()
        pose.position.x = point[0]
        pose.position.y = point[1]
        pose.position.z = point[2]
        pose.orientation.w = 1.0
        msg.pose = pose
        msg.type = Marker.CUBE
        msg.scale.x = .1
        msg.scale.y = .1
        msg.scale.z = .1
        color = ColorRGBA()
        color.r = c[0]
        color.g = c[1]
        color.b = c[2]
        color.a = 1.
        msg.color = color
        msg.header.frame_id = self.frame_id
        msg.header.stamp = self.get_clock().now().to_msg()
        return msg

    def get_avg_color(self, pixels):
        """
        :param pixels: 2d array of colours (bgr)
        :return: average color of the pixels (bgr)
        """
        return np.mean(pixels, axis=(0, 1))

    def object_detection(self, color_frame, depth_frame):
        self.intr = depth_frame.profile.as_video_stream_profile().get_intrinsics()
        self.get_logger().debug(f"Intrinsics: {self.intr}")
        self.get_logger().debug("OBJECT DETECTION CALLBACK")
        color_image = np.asanyarray(color_frame.get_data())
        self.get_logger().debug(f"color frame shape: {color_image.shape}")
        results = self.model.predict(color_image)
        boxes = results[0].boxes.cpu().numpy()
        markers = MarkerArray()

        for box in boxes:
            xmin, ymin, xmax, ymax = box.xyxy.flatten()
            im_width, im_height = xmax-xmin, ymax-ymin
            confidence = box.conf

            if confidence > self.confidence_threshold:
                self.get_logger().debug("FOUND BOX")
                cx, cy = int(xmin) + int(im_width // 2), int(ymin) + int(im_height // 2)
                area_of_interest = color_image[int(ymin):int(ymax), int(xmin):int(xmax)]
                self.get_logger().debug(f"Center: {cx}, {cy}")
                depth = depth_frame.get_distance(cx,cy)
                point = rs.rs2_deproject_pixel_to_point(self.intr, [float(cx), float(cy)], depth)
                self.get_logger().debug(f"Point: {point}")
                color = self.get_avg_color(area_of_interest)[::-1] / 255
                self.get_logger().debug(f"Color: {color}")
                markers.markers.append(self.get_marker(point, color))

                if self.show:
                    cv2.rectangle(color_image, (int(xmin), int(ymin)), (int(xmax), int(ymax)), (0,0,0), 2)
                    cv2.circle(color_image, (cx, cy), 5, (0, 0, 255), -1)
                    cv2.putText(color_image, "Cube", (int(xmin), int(ymin) - 30), cv2.FONT_HERSHEY_SIMPLEX, .5, (0, 0, 255))
                    cv2.putText(color_image, f'Depth: {depth:.2f}', (int(xmin), int(ymin) - 20), cv2.FONT_HERSHEY_SIMPLEX, .5, (0, 0, 255))
        self.obj_pub.publish(markers)

        if self.show:
            cv2.imshow("Object Detection", color_image)
            cv2.waitKey(0)
