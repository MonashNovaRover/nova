__package__ = "autonomous"
import cv2
import time
import numpy as np
from rclpy.node import Node
from visualization_msgs.msg import Marker
from geometry_msgs.msg import Pose
from std_msgs.msg import ColorRGBA
import autonomous.object_detection.config_files as models
from ultralytics import YOLO
import pyrealsense2 as rs
import os

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
        super().__init__("object_image_pub")
        self.obj_pub = self.create_publisher(Marker, "object", 10)
        # Get full path to model from the obj_detect package using os
        model_path = os.path.join(os.path.dirname(models.__file__), "10_mar_cubes_nano.pt")
        
        self.model = YOLO(model_path) 
        self.frame_id = frame_id
        self.intr = intrinsics
        self.show = show
        self.confidence_threshold = confidence_threshold

    def pub_marker(self, point, c: tuple) -> None:
        """
        :params: c is color tuple (r,g,b) between 0 and 1
        """
        msg = Marker()
        pose = Pose()
        pose.position.y = point[0]
        pose.position.z = point[1]
        pose.position.x = point[2]
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
        color.a = 255.
        msg.color = color
        msg.header.frame_id = self.frame_id
        msg.header.stamp = self.get_clock().now().to_msg()
        self.obj_pub.publish(msg)

    def object_detection(self, color_frame, depth_frame):
        color_image = np.asanyarray(color_frame.get_data())
        results = self.model(color_image)

        frame_count += 1
        total_frames += 1

        for box in results.boxes:
            xmin, ymin, xmax, ymax = box.xyxy[0]

            if box.conf > self.confidence_threshold:
                cx, cy = int(xmin) + (int(xmax) - int(xmin))//2, int(ymin) + (int(ymax) - int(ymin))//2
                depth = depth_frame.get_distance(cx,cy)
                point = rs.rs2_deproject_pixel_to_point(self.intr, [float(cx), float(cy)], depth)
                self.pub_marker(point, )
                self.obj_pub.publish(point[0], point[1], point[2], (.1,.2,.3))

                if self.show:
                    cv2.rectangle(color_image, (int(xmin), int(ymin)), (int(xmax), int(ymax)), (0,0,0), 2)
                    cv2.putText(color_image, "Cube", (int(xmin), int(ymin) - 30), cv2.FONT_HERSHEY_SIMPLEX, .5, (0, 0, 255))
                    cv2.putText(color_image, f'Depth: {depth:.2f}', (int(xmin), int(ymin) - 20), cv2.FONT_HERSHEY_SIMPLEX, .5, (0, 0, 255))

                    if frame_count >= 30:
                        end = time.time_ns()
                        fps = 1000000000 * frame_count / (end - start)
                        frame_count = 0
                        start = time.time_ns()

                    if fps > 0:
                        fps_label = f'FPS: {fps:.2f}'
                        cv2.putText(color_image, fps_label, (10, 25), cv2.FONT_HERSHEY_SIMPLEX, 1, (0, 0, 255), 2)

                        cv2.imshow("output", color_image)