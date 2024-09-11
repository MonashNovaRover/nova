from abc import ABC, abstractmethod

from rclpy.node import Node

from rclpy import qos
from std_srvs.srv import Empty

from camera_msgs.msg import Cameras


class BaseCameraDirectoryService(Node, ABC):
    def __init__(self, *args, **kwargs):
        super().__init__(node_name="camera_directory", *args, **kwargs)

        self.get_logger().info("Creating camera directory publishers...")
        self._cameras_publisher = self.create_publisher(
            Cameras,
            "/camera_directory/cameras",
            qos.QoSProfile(
                history=qos.HistoryPolicy.KEEP_LAST,
                depth=1,
                reliability=qos.ReliabilityPolicy.RELIABLE,
                durability=qos.DurabilityPolicy.TRANSIENT_LOCAL,
            ),
        )

        self.get_logger().info("Creating camera directory services...")
        self.create_service(
            Empty,
            "/camera_directory/discover",
            self.__discover_callback,
        )

    def __discover_callback(
        self,
        request: Empty.Request,
        response: Empty.Response,
    ) -> Empty.Response:
        self._discover_cameras()
        return response

    @abstractmethod
    def _discover_cameras(self) -> None:
        ...
