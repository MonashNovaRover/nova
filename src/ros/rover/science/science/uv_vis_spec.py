#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team
This file contains the ROS2 publisher code for the actuator limit switch.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ PACKAGE:     electronics
AUTHOR(S):   Bailey Chessum
CREATION:    2/05/2024
EDITED:      2/05/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import rclpy
from rclpy.node import Node
import cv2
import math
from typing import List

# https://answers.ros.org/question/325559/how-can-we-actually-use-float32multiarray-to-publish-2d-array-using-python/
from nova_interfaces.msg import UVVisSpecData


def rgb_to_luminance(rgb: [int, int, int]) -> float:
    """ Converts a 3 colour channel pixel value to a single luminance value. The max value for an RBG array element
    is 255, so the max value of luminance is sqrt(3*255*255) = 441.67295593
    :param rgb: The colour values from the image
    :return: a luminance value from 0 to 441.67295593
    """
    r, g, b = (float(x) for x in rgb)
    return math.sqrt(r*r + g*g + b*b)


class UVVisSpecPublisher(Node):

    def __init__(self):
        super().__init__("uv_vis_spec")

        # Print initialisation information
        self.get_logger().warning("\033[92;1mInitialising the UV Vis Spectrometer class.\033[0m")

        # publisher to publish the data from the kilns.
        self.publisher = self.create_publisher(UVVisSpecData, "/science/uv_vis_spec_data", 1)

        # declare parameters
        self.declare_parameter("camera_port", 0)

        self.camera = cv2.VideoCapture(self.get_parameter("camera_port").value)
        self.camera.set(cv2.CAP_PROP_BUFFERSIZE, 1)     # Only consider 1 frame at a time

        self.create_timer(0.02, self.__get_image)

    def __get_image(self):
        self.camera.grab()
        success, video_frame = self.camera.read()

        # Ensure the video was successfully retrieved
        if not success or len(video_frame) == 0:
            self.get_logger().warn("UV Vis Spec failed to get frame from camera.")
            return

        # Sample a row from the image if it was valid
        middle_row_index = len(video_frame) // 2
        middle_row = video_frame[middle_row_index]

        # Publish it
        self.publish_reading([rgb_to_luminance(x) for x in middle_row])

    def publish_reading(self, row: List[float]) -> None:
        # Construct a message containing the row
        msg = UVVisSpecData()
        msg.luminance = row

        # Publish to the topic
        self.publisher.publish(msg)

    def __del__(self):
        self.camera.release()


# The main code that executes when starting
def main(args=None):
    # Create the publisher
    rclpy.init(args=args)
    publisher = UVVisSpecPublisher()
    rclpy.spin(publisher)

    #  Clean up when complete
    publisher.destroy_node()
    rclpy.shutdown()

    del publisher


# Called when the script executes
if __name__ == "__main__":
    main()














