#!/usr/bin/env python3

"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Provides a service to republish the most
    recent image on a different topic, where it
    can be captured on the base station.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: image_republisher
TOPICS:
  - subscriber: /oak/rgb/image_raw [Image]
  - publisher: /image_capture/image [Image]
SERVICES: 
  - /image_capture/republish [Empty]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    nova_cube_localisation
AUTHOR(S):	Max Tory
CREATION:	08/03/2024
EDITED:		08/03/2024
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

import rclpy
from rclpy.node import Node

# msg types
from std_srvs.srv import Empty
from sensor_msgs.msg import Image


class ImageCapture(Node):
    """
    Listens to video feed published on rover. Publishes the most recent frame on a service trigger.
    """
    def __init__(self):
        super().__init__("image_capture")

        self.sub_raw_image = self.create_subscription(Image, "/image", self.cb_save_image, 10)
        self.pub_image = self.create_publisher(Image, "~/image", 10)

        self.last_image : Image = None
        self.srv_republish_image = self.create_service(Empty, "~/republish", self.republish_image)

    def republish_image(self, req, res):
        self.pub_image.publish(self.last_image)
        return res

    def cb_save_image(self, msg):
        self.last_image = msg


def main(args=None):
    rclpy.init(args=args)
    node = ImageCapture()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
