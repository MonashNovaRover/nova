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
from science_interfaces.msg import UVVisSpecData
from std_srvs.srv import Trigger


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
        """Initialize the UV Vis Spectrometer node with parameters and services."""
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
        self.__range = self.declare_parameter("range", 0.1)
        # The period at which the camera is sampled
        self.declare_parameter("period", 0.05)

        # Defines the range of columns to use
        self.__col_start = self.declare_parameter("col_start", 0.4)
        self.__col_end = self.declare_parameter("col_end", 0.6)

        # Try get the camera
        self.__timer = None
        self.camera = None
        self.__is_running = False
        self.__camera_node = None  # Will be set when camera is discovered

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

        # Create stop/start services
        self.stop_service = self.create_service(Trigger, "/science/uv_vis_spec/stop", self.__stop_callback)
        self.start_service = self.create_service(Trigger, "/science/uv_vis_spec/start", self.__start_callback)

    def __begin(self, cameras: Cameras):
        """Callback when camera directory is received. Finds and initializes the spectroscope camera."""
        # Don't restart if camera is already running or was intentionally stopped
        if self.camera or (self.__camera_node and not self.__is_running):
            return

        camera_node = next((typing.cast(Camera, camera).node for camera in cameras.cameras if
                            typing.cast(Camera, camera).serial == "science_spectroscope"), None)
        if camera_node:
            self.get_logger().info(f"uv_vis camera found at {camera_node}.")
        else:
            self.get_logger().warn("No uv_vis camera was found. UV Vis Spec. not running.")
            return

        # Store the camera_node for later use by start service
        self.__camera_node = camera_node

        # Open camera and start capture using helper method
        if self.__start_camera_capture():
            self.get_logger().info("Beginning UV Vis Spec.")
            self.__is_running = True
        else:
            self.get_logger().error("Failed to start UV Vis Spec camera")

    def __start_camera_capture(self) -> bool:
        """
        Opens the camera and starts the capture timer.
        Assumes self.__camera_node has been set.
        Returns True if successful, False otherwise.
        """
        if not self.__camera_node:
            self.get_logger().error("No camera node available")
            return False

        try:
            # Open camera (same logic from __begin)
            self.camera = cv2.VideoCapture(int(self.__camera_node.removeprefix("/dev/video")))
            self.camera.set(cv2.CAP_PROP_BUFFERSIZE, 1)

            # Start the timer for frame capture
            self.__timer = self.create_timer(
                float(self.get_parameter("period").value),
                self.__get_image
            )

            return True
        except Exception as e:
            self.get_logger().error(f"Failed to start camera: {e}")
            return False

    def __stop_callback(self, request: Trigger.Request, response: Trigger.Response) -> Trigger.Response:
        """Service callback to stop the camera feed and release the camera."""
        if not self.__is_running:
            self.get_logger().warn("Stop service called but camera is already stopped")
            response.success = False
            response.message = "UV Vis Spec camera is already stopped"
            return response

        # Stop the timer
        if self.__timer:
            self.__timer.cancel()
            self.__timer = None

        # Release the camera
        if self.camera:
            self.camera.release()
            self.camera = None

        self.__is_running = False
        self.get_logger().info("UV Vis Spec camera stopped and released")

        response.success = True
        response.message = "Camera stopped successfully"
        return response

    def __start_callback(self, request: Trigger.Request, response: Trigger.Response) -> Trigger.Response:
        """Service callback to restart the camera feed."""
        if self.__is_running:
            self.get_logger().warn("Start service called but camera is already running")
            response.success = False
            response.message = "UV Vis Spec camera is already running"
            return response

        # Reuse the camera opening logic
        if self.__start_camera_capture():
            self.__is_running = True
            self.get_logger().info("UV Vis Spec camera restarted")
            response.success = True
            response.message = "Camera started successfully"
        else:
            self.get_logger().error("Start service failed to open camera")
            response.success = False
            response.message = "Failed to open camera"

        return response

    def __get_image(self):
        """Capture a frame from the camera, extract luminance data, and publish it."""
        # Safety check: only proceed if camera is running
        if not self.__is_running or not self.camera:
            return

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
        """Publish luminance readings to the UV Vis Spec data topic."""
        # Construct a message containing the row
        msg = UVVisSpecData()
        msg.luminance = row

        # Publish to the topic
        self.publisher.publish(msg)

    def __del__(self):
        """Destructor to release the camera when the node is destroyed."""
        self.camera.release()


# The main code that executes when starting
def main(args=None):
    """Main entry point for the UV Vis Spectrometer node."""
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
