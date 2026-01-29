from typing import cast

import rclpy
from rclpy import Parameter

from camera_msgs.msg import Camera, Cameras, Encoder, Encoders

from cameras2.base_camera_directory_service import BaseCameraDirectoryService
from cameras2.camera_scanner import CameraScanner

from multiprocessing import Process
import multiprocessing as mp
import signal
import os

from oakenc.oakenc import run, MessageType

class OakCameraDirectoryService(BaseCameraDirectoryService):
    """
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Monash Nova Rover Team

    This service exposes information about OAK Cameras
    connected to the system.
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    NODE: camera_directory
    TOPICS:
      - /camera_directory/cameras [camera_msgs/Cameras] (transient local)
    SERVICES:
      - /camera_directory/discover [std_srvs/Empty]
    ACTIONS: None
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    PACKAGE: 	oakenc
    AUTHOR(S):	Orlando Chamberlain
    CREATION:	28/01/2026
    EDITED:		28/01/2026
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    """

    def __init__(self):
        super().__init__(
            node_name="oak_camera_directory",
            allow_undeclared_parameters=True,
            automatically_declare_parameters_from_overrides=True,
        )

        self._cameras = []
        self._encoders = []

        mp.set_start_method("spawn") # depthai hangs with fork :/

        self.pipe, pipeDepthai = mp.Pipe()
        self.p = Process(target=run, args=(pipeDepthai,))
        self.p.start()

        self.timer = self.create_timer(1.0, self.spin_once)


    def is_alive(self):
        return self.p.is_alive()

    def spin_once(self):
        if self.pipe.poll():
            type_, value = self.pipe.recv()

            match type_:
                case MessageType.CAMERAS:

                    self.get_logger().info("Got Cameras:")
                    for camera in value:
                        self.get_logger().info(f"\t{camera}")

                    self._cameras = value
                    self._publish_cameras()
                case _:
                    self.get_logger().info(f"{type_}, {value}")

    def destroy_node(self) -> bool:
        os.kill(self.p.pid, signal.SIGUSR1) # keyboard interrupt
        self.p.join()
        return super().destroy_node()


    def _discover_cameras(self) -> None:
        pass



    def _publish_cameras(self) -> None:
        self._cameras_publisher.publish(
            Cameras(
                cameras=[camera for camera  in self._cameras]
                )
        )


def main(args=None):
    rclpy.init(args=args)
    node = OakCameraDirectoryService()

    while node.is_alive():
        rclpy.spin(node)

    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
