from typing import cast

import rclpy
from rclpy import Parameter

from camera_msgs.msg import Camera, Cameras
from std_srvs.srv import Empty

from cameras2.base_camera_directory_service import BaseCameraDirectoryService
from cameras2.camera_scanner import CameraScanner
SERVICES = ( "/v4l_camera_directory", "/oak_camera_directory",
        )

class CameraRepublisherService(BaseCameraDirectoryService):
    """
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Monash Nova Rover Team

    This service combines lists of cameras from several
    camera directory services.
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    NODE: camera_directory
    TOPICS:
      - /camera_directory/cameras [camera_msgs/Cameras] (transient local)
    SERVICES:
      - /camera_directory/discover [std_srvs/Empty]
    ACTIONS: None
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    PACKAGE: 	cameras2
    AUTHOR(S):	Orlando Chamberlain
    CREATION:	29/01/2026
    EDITED:		29/01/2026
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    """

    def __init__(self):
        super().__init__(
            allow_undeclared_parameters=True,
            automatically_declare_parameters_from_overrides=True,
        )

        self._camera_sets = {}
        self._discover_clients = {}
        for service in SERVICES:
            # sub
            self._camera_sets[service] = Cameras(cameras=[])
            def cb(service_, cameras):
                self.get_logger().info(f"{service_} sent {cameras}")
                self._camera_sets[service_] = cameras
                self._publish_cameras()
            self.create_subscription(
                    Cameras,
                    service+"/cameras",
                    # service=service to capture the value not variable
                    lambda c, service=service: cb(service, c),
                    1 #RELIABLE
                    )
            # call
            self._discover_clients[service] = self.create_client(
                    Empty,
                    service+"/discover"
                    )

        self._cameras = {}

    def destroy_node(self) -> bool:
        return super().destroy_node()

    def _discover_cameras(self) -> None:
        for service in self._discover_clients:
            self._discover_clients[service].call(Empty())
        pass



    def _publish_cameras(self) -> None:
        allCams = []
        for service in self._camera_sets:
            cameras = self._camera_sets[service].cameras
            self.get_logger().info(f"cameras: {cameras}")
            allCams.extend(cameras)
            self.get_logger().info(f"allCams: {allCams}")

        self._cameras_publisher.publish(
                Cameras(cameras=allCams)
        )


def main(args=None):
    rclpy.init(args=args)
    node = CameraRepublisherService()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
