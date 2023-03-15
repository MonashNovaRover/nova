#!/usr/bin/env python3
from cv_bridge import CvBridge
import rclpy
from rclpy.node import Node

from sensor_msgs.msg import Image
import cv2, os, time


class ImageSaver(Node):
    def __init__(self):
        super().__init__("image_saver")

        self.sub_imgs = self.create_subscription(Image, "depth_camera/map/image", self.cb_save_image, 10)
        self.cv_bridge : CvBridge = CvBridge()

        self.image_path = os.path.expanduser("~/training_images")
        if not os.path.exists(self.image_path):
            os.mkdir(self.image_path)
    
    def cb_save_image(self, msg):
        self.get_logger().info("saving image")
        cv_image = self.cv_bridge.imgmsg_to_cv2(msg, "8UC3")
        path = os.path.join(self.image_path, str(time.perf_counter()) + ".jpg")
        cv2.imwrite(path, cv_image)
        self.get_logger().info(f"wrote to {path}")


if __name__ == "__main__":
    rclpy.init()
    saver = ImageSaver()
    rclpy.spin(saver)
    saver.destroy_node()
    rclpy.shutdown()