#!/usr/bin/env python3
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Image

import argparse
import cv2, os, time
import time
from cv_bridge import CvBridge

class ImageSaver(Node):
    def __init__(self, fps, out_path):
        super().__init__("image_saver")
        self.fps = fps
        self.rate = 1.0 / self.fps
        self.sub_imgs = self.create_subscription(Image, "depth_camera/d435_1/image", self.cb_save_image, 10)
        self.cv_bridge : CvBridge = CvBridge()
        self.last_update = time.time() 
        self.image_path = os.path.expanduser("~/" + out_path)

        if not os.path.exists(self.image_path):
            os.mkdir(self.image_path)
    
    def cb_save_image(self, msg):
        # only run the save function every RATE seconds 
        if time.time() - self.last_update < self.rate:
            print("got image, not saving...")
            return
        self.last_update = time.time()

        self.get_logger().info("saving image")
        cv_image = self.cv_bridge.imgmsg_to_cv2(msg, "8UC3")
        path = os.path.join(self.image_path, str(time.perf_counter()) + ".jpg")
        cv2.imwrite(path, cv_image)
        self.get_logger().info(f"wrote to {path}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--fps", default=1, type=int)
    parser.add_argument("--out_dir", default="training_images", type=str)
    args = parser.parse_args()

    rclpy.init()

    print(f"Running image save with fps={args.fps}, out_dir=~/{args.out_dir}")

    saver = ImageSaver(float(args.fps), args.out_dir)
    rclpy.spin(saver)
    saver.destroy_node()
    rclpy.shutdown()
