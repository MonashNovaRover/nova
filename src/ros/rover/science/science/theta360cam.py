#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
Code for interfacing with the Ricoh Theta S 360
camera to take equirectangular images and publish
them on ROS.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
AUTHOR(S):   Bailey Chessum
CREATION:    17/05/2024
EDITED:      17/05/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import cv2
from cv_bridge import CvBridge
import locale
import logging
import subprocess
import sys
import os
import gphoto2 as gp
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import CompressedImage
from std_srvs.srv import Empty


class Theta360CamPublisher(Node):

    def __init__(self):
        super().__init__('theta360cam')

        # This allows us to actually see errors from the libgphotos2 c library, rather than just arbitrary int errors
        locale.setlocale(locale.LC_ALL, '')
        logging.basicConfig(
            format='%(levelname)s: %(name)s: %(message)s', level=logging.WARNING)
        callback_obj = gp.check_result(gp.use_python_logging())

        # Create camera object
        self.__camera = gp.Camera()
        self.__camera.init()

        # Create service and publisher for images
        self.create_service(Empty, "/science/theta360cam/capture", self.capture)
        self.__image_publisher = self.create_publisher(CompressedImage, '/science/theta360cam/image', 10)

        self.__bridge = CvBridge()

        self.get_logger().info("360 cam node has been set up")


    def capture(self, request: Empty.Request, response: Empty.Response) -> Empty.Response:
        self.get_logger().info("Capturing image...")

        # Take the image
        file_path = self.__camera.capture(gp.GP_CAPTURE_IMAGE)

        self.get_logger().info(f"Captured \"{file_path.name}\"")
        # Get a path to store the image at
        target = os.path.join('/tmp', file_path.name)

        self.get_logger().info(f"Retrieving to {target}")
        # Get the image from the camera to the above path
        camera_file = self.__camera.file_get(file_path.folder, file_path.name, gp.GP_FILE_TYPE_NORMAL)
        # The API allows for this to be saved to a file
        camera_file.save(target)

        self.get_logger().info(f"Reading with OpenCV and publishing...")
        # Read the data with opencv
        image = cv2.imread(target, cv2.IMREAD_COLOR)
        image_msg = self.__bridge.cv2_to_compressed_imgmsg(image)

        # Publish the image
        self.__image_publisher.publish(image_msg)
        self.get_logger().info(f"Finished publishing {file_path.name}!")

        # We can now share the target
        return response


# The main code that executes when starting
def main(args=None):
    # Create the publisher
    rclpy.init(args=args)
    publisher = Theta360CamPublisher()
    rclpy.spin(publisher)

    #  Clean up when complete
    publisher.destroy_node()
    rclpy.shutdown()


# Called when the script executes
if __name__=="__main__":
    main()

