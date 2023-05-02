from typing import Callable

import gi
import rclpy
from rclpy import Future, qos
from rclpy.logging import LoggingSeverity
from rclpy.node import Node
from rclpy.service import Service
from rclpy.client import Client

gi.require_version("Gst", "1.0")
from gi.repository import Gst, GLib

from camera_msgs.msg import Cameras
from camera_msgs.srv import CameraOperation

from cameras2.camera_webrtc_bin import CameraWebRTCBin


class CameraStreamerService(Node):
    """
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    Monash Nova Rover Team

    This service manages a GStreamer pipeline to
    stream video footage from cameras over WebRTC.
    Consult the repository README for complete setup
    instructions.
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    NODE: camera_streamer
    TOPICS: None
    SERVICES:
      - /camera_streamer/stream/<camera_serial>/start [std_srvs/Empty]
      - /camera_streamer/stream/<camera_serial>/pause [std_srvs/Empty]
      - /camera_streamer/stream/<camera_serial>/stop [std_srvs/Empty]
    ACTIONS: None
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    PACKAGE: 	cameras2
    AUTHOR(S):	Joshua Leivenzon
    CREATION:	25/02/2023
    EDITED:		25/02/2023
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    """

    def __init__(self):
        super().__init__("camera_streamer")

        # Initialize GStreamer.
        self.get_logger().info("Initializing GStreamer...")
        # noinspection PyArgumentList
        Gst.init(None)
        self._gst_pipeline: Gst.Pipeline = Gst.Pipeline.new("camera-server-pipeline")
        self._gst_pipeline.set_state(Gst.State.PLAYING)
        self._gst_pipeline.get_bus().set_sync_handler(self._handle_gst_message, None)
        self._camera_bins: dict[str, CameraWebRTCBin] = {}

        # Create services and clients.
        self.get_logger().info("Creating stream control services...")
        self._create_stream_service("start", self._stream_start)
        self._create_stream_service("pause", self._stream_pause)
        self._create_stream_service("stop", self._stream_stop)

        self._stream_start_client = self._create_stream_client("start")
        self._stream_stop_client = self._create_stream_client("stop")

        # Watch the camera directory, and handle camera availability changes.
        self.get_logger().info("Binding to camera directory service...")
        self._device_nodes: dict[str, str] = {}
        self.create_subscription(
            Cameras,
            "/camera_directory/cameras",
            self._update_cameras,
            qos.QoSProfile(
                history=qos.HistoryPolicy.KEEP_LAST,
                depth=1,
                reliability=qos.ReliabilityPolicy.RELIABLE,
                durability=qos.DurabilityPolicy.TRANSIENT_LOCAL,
            ),
        )

        self.get_logger().info("Ready!")

    def _handle_gst_message(self, bus: Gst.Bus, message: Gst.Message, *user_data) -> Gst.BusSyncReply:
        error: GLib.Error
        debug_info: str
        severity: LoggingSeverity
        match message.type:
            case Gst.MessageType.ERROR:
                error, debug_info = message.parse_error()
                severity = LoggingSeverity.ERROR
            case Gst.MessageType.WARNING:
                error, debug_info = message.parse_warning()
                severity = LoggingSeverity.WARN
            case Gst.MessageType.INFO:
                error, debug_info = message.parse_info()
                severity = LoggingSeverity.WARN
            case _:
                return Gst.BusSyncReply.PASS

        logger = self.get_logger().get_child("GStreamer")
        logger.log(f"{message.src.get_name()}: {error.message}", severity)
        if debug_info:
            logger.log(debug_info, severity)
        return Gst.BusSyncReply.DROP

    def _create_stream_service(
        self,
        srv_name: str,
        callback: Callable[[set[str]], bool],
    ) -> Service:
        def srv_callback(
            request: CameraOperation.Request,
            response: CameraOperation.Response,
        ) -> CameraOperation.Response:
            response.success = callback(
                set(request.serials).intersection(self._device_nodes.keys())
                if request.serials
                else set(self._device_nodes.keys())
            )
            return response

        return self.create_service(
            CameraOperation,
            f"/camera_streamer/stream/{srv_name}",
            srv_callback,
        )

    def _create_stream_client(self, srv_name: str) -> Client:
        return self.create_client(
            CameraOperation,
            f"/camera_streamer/stream/{srv_name}",
        )

    def _register_cameras(self, **kwargs) -> None:
        if not kwargs:
            return

        self.get_logger().info(f"Registering cameras: {', '.join(kwargs.keys())}.")
        self._device_nodes.update(kwargs)

    def _unregister_cameras(self, serials: set[str]) -> None:
        if not serials:
            return

        self.get_logger().info(f"Unregistering cameras: {', '.join(serials)}.")

        def callback(future: Future) -> None:
            for serial in serials:
                del self._device_nodes[serial]

        self._stream_stop_client.call_async(CameraOperation.Request(serials=serials)).add_done_callback(callback)

    def _update_cameras(self, cameras: Cameras) -> None:
        # Determine which of the given ("available") serials are new or updated.
        available_serials = set(camera.serial for camera in cameras.cameras)
        registered_serials = set(self._device_nodes.keys())
        updated_serials = set(
            camera.serial
            for camera in cameras.cameras
            if camera.serial in registered_serials and camera.node != self._device_nodes[camera.serial]
        )

        # Unregister cameras that are no longer available or need updating.
        self._unregister_cameras((registered_serials - available_serials) | updated_serials)

        # Register cameras that are newly available or need updating.
        self._register_cameras(
            **{
                camera.serial: camera.node
                for camera in cameras.cameras
                if camera.serial in (available_serials - registered_serials) | updated_serials
            }
        )

    @staticmethod
    def _create_camera_bin(serial: str, device_node: str) -> CameraWebRTCBin:
        camera_bin = CameraWebRTCBin(
            serial,
            device_node,
            # Hardcoded values optimized for several Microsoft LifeCam 3000s on a USB hub.
            # TODO: Use ROS parameters for per-device capability filter attributes.
            width=640,
            fps=10,
        )

        return camera_bin

    def _stream_start(self, serials: set[str]) -> bool:
        for serial in serials:
            camera_bin = self._camera_bins.get(serial)
            if camera_bin is None:
                self.get_logger().info(f"Starting stream for camera {serial}.")
                camera_bin = self._create_camera_bin(serial, self._device_nodes[serial])
                self._gst_pipeline.add(camera_bin.bin)
                self._camera_bins[serial] = camera_bin
            else:
                self.get_logger().info(f"Resuming stream for camera {serial}.")

            camera_bin.bin.set_state(Gst.State.PLAYING)
        return True

    def _stream_pause(self, serials: set[str]) -> bool:
        success = bool(serials)
        for serial in serials:
            camera_bin = self._camera_bins.get(serial)
            if camera_bin is None:
                success = False
                continue

            self.get_logger().info(f"Pausing stream for camera {serial}.")
            camera_bin.bin.set_state(Gst.State.PAUSED)
        return success

    def _stream_stop(self, serials: set[str]) -> bool:
        success = bool(serials)
        for serial in serials:
            camera_bin = self._camera_bins.pop(serial, None)
            if camera_bin is None:
                success = False
                continue

            self.get_logger().info(f"Stopping stream for camera {serial}.")
            camera_bin.bin.set_state(Gst.State.NULL)
            self._gst_pipeline.remove(camera_bin.bin)
        return success


def main(args=None):
    rclpy.init(args=args)
    server = CameraStreamerService()
    rclpy.spin(server)
    server.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
