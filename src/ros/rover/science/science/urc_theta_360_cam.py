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
EDITED:      20/05/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import cv2
import gphoto2._camera
from cv_bridge import CvBridge
import locale
import logging
import os
import gphoto2 as gp
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import CompressedImage
from std_srvs.srv import Trigger
from typing import Optional


class URCTheta360CamPublisher(Node):
    """ Node to capture images from the 360 cam using MTP (through the gphoto2 library) and publish them as
    compressed images over ROS.

    ! This assumes there are no other MTP devices connected to the rover! Otherwise, you'll need to add more logic to
    specify the correct port.
    """

    def __init__(self):
        super().__init__('theta360cam')

        # This allows us to actually see errors from the libgphotos2 c library, rather than just arbitrary int errors
        # TODO: use ROS logging somehow
        locale.setlocale(locale.LC_ALL, '')
        logging.basicConfig(
            format='%(levelname)s: %(name)s: %(message)s', level=logging.WARNING)
        callback_obj = gp.check_result(gp.use_python_logging())

        self.__camera: Optional[gphoto2.Camera] = None
        self.init_camera()


        # Create service and publisher for images
        self.create_service(Trigger, "/science/theta360cam/capture", self.capture)
        self.__image_publisher = self.create_publisher(CompressedImage, '/science/theta360cam/image', 10)

        self.__bridge = CvBridge()
        self.create_timer(30, self.wake_camera)

        self.get_logger().info("theta360cam >>> 360 cam node has been set up")

    def init_camera(self):
        # If the camera was previously opened, close it.
        if self.__camera is not None:
            self.__camera.exit()
            self.__camera = None

        # Create camera object
        # ! If you get the [-105] unknown model, it means it can't find the camera. Make sure it's turned on.
        self.__camera = gp.Camera()
        self.__camera.init()

        text = self.__camera.get_summary()
        self.get_logger().info(f"Camera summary:\n{str(text)}")

    def capture(self, request: Trigger.Request, response: Trigger.Response) -> Trigger.Response:
        """ Called with the /science/theta360cam/capture service. Expects everything to be synchronous, where the camera
        as a resource isn't under contention. """
        self.get_logger().info("Capturing image...")

        # forward declaration because I don't trust python
        file_path = None

        try:
            # Take the image
            file_path = self.__camera.capture(gp.GP_CAPTURE_IMAGE, )

            self.get_logger().info(f"Captured \"{file_path.name}\"")
            # Get a path to store the image at
            target = os.path.join('/tmp', file_path.name)

            self.get_logger().info(f"Downloading and saving the image to \"{target}\"")
            # Get the image from the camera to the above path
            camera_file = self.__camera.file_get(file_path.folder, file_path.name, gp.GP_FILE_TYPE_NORMAL)
            # The API allows for this to be saved to a file
            camera_file.save(target)

            self.get_logger().info(f"Reading the image with OpenCV and publishing...")
            # Read the data with opencv
            image = cv2.imread(target, cv2.IMREAD_COLOR)
            image_msg = self.__bridge.cv2_to_compressed_imgmsg(image)

            # Publish the image
            self.__image_publisher.publish(image_msg)
            self.get_logger().info(f"Finished publishing {file_path.name}!\n")
        except Exception as e:
            self.get_logger().error("An exception occurred while attempting to capture an image.")
            self.get_logger().error(str(e))

            # Try to re-initialise the camera
            try:
                self.init_camera()
                return self.capture(request, response)
            except gp.GPhoto2Error as e:
                self.get_logger().error("Failed to re-initialise the camera.")
                self.get_logger().error(str(e))

                # This try-except is just so feedback is displayed properly in GUI, and we fail loudly
                response.success = False
                response.message = str(e)
                return response

        # otherwise, report success
        response.success = True
        response.message = f"Captured {file_path.name}"

        # We can now share the target
        return response

    def wake_camera(self):
        if self.__camera is None:
            return

        battery = self.__camera.get_single_config("batterylevel")
        self.get_logger().info(f"Battery level: {battery.get_value()}")

    def on_shutdown(self):
        if self.__camera is not None:
            self.__camera.exit()


# The main code that executes when starting
def main(args=None):
    # Create the publisher
    rclpy.init(args=args)
    publisher = URCTheta360CamPublisher()
    rclpy.spin(publisher)

    #  Clean up when complete
    publisher.destroy_node()
    rclpy.shutdown()


# Called when the script executes
if __name__=="__main__":
    main()

