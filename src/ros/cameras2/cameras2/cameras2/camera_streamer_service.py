from typing import Callable

import gi
import rclpy
from rclpy import qos
from rclpy.node import Node
from rclpy.service import Service
from rclpy.client import Client

gi.require_version("Gst", "1.0")
from gi.repository import Gst

from camera_msgs.msg import Camera, Cameras
from camera_msgs.srv import CameraOperation


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
        self._gst_bins: dict[str, Gst.Bin] = {}

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
            self._register_new_cameras,
            qos.QoSProfile(
                history=qos.HistoryPolicy.KEEP_LAST,
                depth=1,
                reliability=qos.ReliabilityPolicy.RELIABLE,
                durability=qos.DurabilityPolicy.TRANSIENT_LOCAL,
            ),
        )

        self.get_logger().info("Ready!")

    def _create_stream_service(
        self,
        srv_name: str,
        callback: Callable[[str], bool],
    ) -> Service:
        def srv_callback(
            request: CameraOperation.Request,
            response: CameraOperation.Response,
        ) -> CameraOperation.Response:
            response.success = (
                callback(request.serial)
                if request.serial in self._device_nodes
                else False
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

    def _register_camera(self, camera: Camera) -> None:
        self.get_logger().info(f"Registering camera {camera.serial}.")
        self._device_nodes[camera.serial] = camera.node
        self._stream_start_client.call_async(
            CameraOperation.Request(serial=camera.serial)
        )

    def _unregister_camera(self, serial: str) -> None:
        self.get_logger().info(f"Unregistering camera {serial}.")
        self._stream_stop_client.call_async(
            CameraOperation.Request(serial=serial)
        ).add_done_callback(lambda future: self._device_nodes.pop(serial))

    def _register_new_cameras(self, cameras: Cameras) -> None:
        """
        Register new cameras, and remove old ones.
        :param cameras: The current list of cameras.
        """
        for camera in cameras.cameras:
            if camera.serial not in self._device_nodes:
                self._register_camera(camera)

        available_serials = (camera.serial for camera in cameras.cameras)
        registered_serials = self._device_nodes.keys()
        for serial in registered_serials - available_serials:
            self._unregister_camera(serial)

    @staticmethod
    def _create_camera_bin(serial: str, device_node: str) -> Gst.Bin:
        # Create the bin and elements.
        gst_bin: Gst.Bin = Gst.Bin.new(f"camera-{serial}-bin")

        source = Gst.ElementFactory.make("v4l2src", "source")
        # TODO: Cameras may not always support JPEG. Fall back to another format if this fails.
        # videoconvert is recommended here (when not using jpegdec) as v4l2src has unknown capabilities.
        # https://gstreamer.freedesktop.org/documentation/tutorials/basic/handy-elements.html#videoconvert
        # videoconvert = Gst.ElementFactory.make("videoconvert", "videoconvert")
        decoder = Gst.ElementFactory.make("jpegdec", "decoder")
        sink = Gst.ElementFactory.make("webrtcsink", "sink")

        # Configure the elements.
        # # Source
        source.props.device = device_node

        # # Sink
        # ## WebRTC settings
        # TODO: Move to gcc (Google Congestion Control) when GStreamer 1.22 is released.
        sink.props.congestion_control = "homegrown"

        # ## Metadata
        meta: Gst.Structure = Gst.Structure.new_empty("meta")
        meta.set_value("serial", serial)
        sink.props.meta = meta

        # Add the elements to the bin, and link them.
        elements = [source, decoder, sink]
        gst_bin.add(*elements)
        # noinspection PyUnresolvedReferences
        Gst.Element.link_many(*elements)

        return gst_bin

    def _stream_start(self, serial: str) -> bool:
        gst_bin = self._gst_bins.get(serial)
        if gst_bin is None:
            gst_bin = self._create_camera_bin(serial, self._device_nodes[serial])
            self._gst_pipeline.add(gst_bin)
            self._gst_bins[serial] = gst_bin

        self.get_logger().info(f"Starting stream for camera {serial}.")
        gst_bin.set_state(Gst.State.PLAYING)

        return True

    def _stream_pause(self, serial: str) -> bool:
        gst_bin = self._gst_bins.get(serial)
        if gst_bin is None:
            return False

        self.get_logger().info(f"Pausing stream for camera {serial}.")
        gst_bin.set_state(Gst.State.PAUSED)

        return True

    def _stream_stop(self, serial: str) -> bool:
        gst_bin = self._gst_bins.pop(serial, None)
        if gst_bin is None:
            return False

        self.get_logger().info(f"Stopping stream for camera {serial}.")
        gst_bin.set_state(Gst.State.NULL)
        self._gst_pipeline.remove(gst_bin)

        return True


def main(args=None):
    rclpy.init(args=args)
    server = CameraStreamerService()
    rclpy.spin(server)
    server.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
