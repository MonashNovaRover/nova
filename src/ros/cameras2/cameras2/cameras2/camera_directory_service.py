import rclpy
from rclpy.node import Node
from rclpy import qos
from std_srvs.srv import Empty

from camera_msgs.msg import Camera, Cameras

from cameras2 import cameras


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
    TOPICS:
      - /camera_directory/cameras [camera_msgs/Cameras] (transient local)
    SERVICES:
      - /camera_directory/discover [std_srvs/Empty]
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
            self._discover_callback,
        )

        self._discover_cameras()
        self._start_watching_cameras()

    def destroy_node(self) -> bool:
        self._stop_watching_cameras()
        return super().destroy_node()

    def _discover_cameras(self) -> None:
        self.get_logger().info("Searching for cameras...")
        self._cameras: dict[str, str] = cameras.find_cameras()
        self.get_logger().info(
            "Cameras found:\n"
            + "\n".join(
                f"- {serial} at {device_node}"
                for serial, device_node in self._cameras.items()
            )
        )
        self._publish_cameras()

    def _start_watching_cameras(self) -> None:
        self.get_logger().info("Starting background camera discovery...")

        def callback(added: bool, serial: str, device_node: str) -> None:
            if added:
                self.get_logger().info(
                    f"New camera discovered: {serial} at {device_node}"
                )
                self._cameras[serial] = device_node
            else:
                self.get_logger().info(f"Camera removed: {serial}")
                self._cameras.pop(serial, None)
            self._publish_cameras()

        self._camera_watch_stop_callback = cameras.watch_cameras(callback)

    def _stop_watching_cameras(self) -> None:
        self.get_logger().info("Stopping background camera discovery...")
        self._camera_watch_stop_callback()

    def _publish_cameras(self) -> None:
        self._cameras_publisher.publish(
            Cameras(
                cameras=[
                    Camera(serial=serial, node=device_node)
                    for serial, device_node in self._cameras.items()
                ]
            )
        )

    def _discover_callback(
        self,
        request: Empty.Request,
        response: Empty.Response,
    ) -> Empty.Response:
        self._discover_cameras()
        return response


def main(args=None):
    rclpy.init(args=args)
    node = CameraDirectoryService()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
