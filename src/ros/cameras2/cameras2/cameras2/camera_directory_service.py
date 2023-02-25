import rclpy
from rclpy.node import Node
from std_srvs.srv import Empty

from camera_msgs.msg import Camera
from camera_msgs.srv import GetCameras

from cameras2.cameras import find_cameras


class CameraDirectoryService(Node):
    """
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Monash Nova Rover Team

    This service exposes information about cameras
    connected to the system.
    Either the entire list of cameras can be retrieved,
    or specific cameras can be searched for via their
    serial numbers.
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    NODE: camera_directory
    TOPICS: None
    SERVICES:
      - /camera_directory/discover [std_srvs/Empty]
      - /camera_directory/get_cameras [camera_msgs/GetCameras]
    ACTIONS: None
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    PACKAGE: 	cameras2
    AUTHOR(S):	Joshua Leivenzon
    CREATION:	25/02/2023
    EDITED:		25/02/2023
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    """

    def __init__(self):
        super().__init__("camera_directory")
        self._discover_cameras()
        self.create_service(
            Empty,
            "/camera_directory/discover",
            self._discover_callback,
        )
        self.create_service(
            GetCameras,
            "/camera_directory/get_cameras",
            self._get_cameras_callback,
        )

    def _discover_cameras(self) -> None:
        # Find all cameras connected to the system.
        self._cameras = find_cameras()

    def _discover_callback(
        self,
        request: Empty.Request,
        response: Empty.Response,
    ) -> Empty.Response:
        self._discover_cameras()
        return response

    def _get_cameras_callback(
        self,
        request: GetCameras.Request,
        response: GetCameras.Response,
    ) -> GetCameras.Response:
        serials = request.serials if request.serials else self._cameras.keys()
        response.cameras = [
            Camera(serial=serial, node=self._cameras[serial])
            for serial in serials
            if serial in self._cameras
        ]
        return response


def main(args=None):
    rclpy.init(args=args)
    node = CameraDirectoryService()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
