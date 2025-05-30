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
import typing

import camera_msgs.msg
import rclpy
from camera_msgs.msg import Camera, Cameras
from rclpy import qos
from rclpy.node import Node
import cv2
import math
from typing import List
from nova_interfaces.msg import UVVisSpecData


def rgb_to_luminance(rgb: [int, int, int]) -> float: # type: ignore
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

        # ./result/bin/ros2 run science uv_vis_spec.py --ros-args -p period:=0.0166666666 -p col_start:=0.425 -p col_end:=0.7 -p row:=0.45 -p range:=0.1 -p port:=

        # declare parameters
        # The row to use when sampling the image, from 0 to 1
        self.__row = self.declare_parameter("row", 0.55)
        # Specifies the size of the range of pixels to vertically average to get a reading
        self.__range = self.declare_parameter("range", 0.05)
        # The period at which the camera is sampled
        self.declare_parameter("period", 0.05)

        # Defines the range of columns to use
        self.__col_start = self.declare_parameter("col_start", 0.05)
        self.__col_end = self.declare_parameter("col_end", 0.95)

        # Try get the camera
        self.__timer = None
        self.camera = None

        self.get_logger().info("Waiting for camera directory service...")
        self.__camera_list_subscription = self.create_subscription(
            camera_msgs.msg.Cameras,
            "camera_directory/cameras",
            self.__begin,
            qos.QoSProfile(
                history=qos.HistoryPolicy.KEEP_LAST,
                depth=1,
                reliability=qos.ReliabilityPolicy.RELIABLE,
                durability=qos.DurabilityPolicy.TRANSIENT_LOCAL,
            ),
        )

    def __begin(self, cameras: Cameras):
        if self.camera:
            return  # TODO: Allow reconnecting the camera

        camera_node = next((typing.cast(Camera, camera).node for camera in cameras.cameras if
                            typing.cast(Camera, camera).serial == "mast_forward"), None)
        if camera_node:
            self.get_logger().info(f"uv_vis camera found at {camera_node}.")
        else:
            self.get_logger().warn("No uv_vis camera was found. UV Vis Spec. not running.")
            return

        self.get_logger().info("Beginning UV Vis Spec.")

        self.camera = cv2.VideoCapture(int(camera_node.removeprefix("/dev/video")))  # TODO: Make port selection more robust
        # Only consider 1 frame at a time, as we likely sample the camera slower than it reads pixels
        self.camera.set(cv2.CAP_PROP_BUFFERSIZE, 1)

        # Start listening for camera frames
        self.__timer = self.create_timer(float(self.get_parameter("period").value), self.__get_image)

    def __get_image(self):
        self.camera.grab()
        success, video_frame = self.camera.read()

        # Ensure the video was successfully retrieved
        if not success or len(video_frame) == 0:
            self.get_logger().warn("UV Vis Spec failed to get frame from camera.")
            return

        # Sample the row at self.__row.value, where 0 corresponds to the top row and 1 corresponds to the bottom row.
        top_row_percent = self.__row.value - 0.5 * self.__range.value
        top_row_index = max(min(math.floor(top_row_percent * len(video_frame)), len(video_frame) - 1), 0)

        bottom_row_percent = self.__row.value + 0.5 * self.__range.value
        bottom_row_index = max(min(math.ceil(bottom_row_percent * len(video_frame)), len(video_frame) - 1), 0)

        if bottom_row_index <= top_row_index:
            bottom_row_index = top_row_index + 1
            # prevent index errors
            if bottom_row_index > len(video_frame):
                bottom_row_index = len(video_frame)
                top_row_index = max(bottom_row_index - 1, 0)

        # Average rows in range
        row_count = max(bottom_row_index - top_row_index, 1)

        # Sample a row from the image if it was valid
        row_length = len(video_frame[top_row_index])
        col_start_index = max(min(math.floor(self.__col_start.value * row_length), row_length - 1), 0)
        col_end_index = max(min(math.ceil(self.__col_end.value * row_length), row_length), col_start_index)

        # Sample and average rows
        reading = [
            sum(
                (rgb_to_luminance(video_frame[r][c]) for r in range(top_row_index, bottom_row_index))
            ) / row_count for c in range(col_start_index, col_end_index)
        ]

        # Publish it
        self.publish_reading(reading)

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
