from typing import Callable, Type, NamedTuple

import gi
import rclpy
from rclpy import qos
from rclpy.node import Node
from rclpy.service import Service
from std_srvs.srv import Empty

gi.require_version("Gst", "1.0")
from gi.repository import Gst

from camera_msgs.msg import Camera, Cameras


class CameraRegistration(NamedTuple):
    services: list[Service] = []


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

        # Keep track of registered cameras.
        self._camera_registrations: dict[str, CameraRegistration] = {}

        # Watch the camera directory, and handle camera availability changes.
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
        camera: Camera,
        srv_name: str,
        callback: Callable[
            [Camera, rclpy.node.SrvTypeRequest, rclpy.node.SrvTypeResponse],
            rclpy.node.SrvTypeResponse,
        ],
        srv_type: Type = Empty,
    ) -> Service:
        return self.create_service(
            srv_type,
            f"/camera_streamer/stream/camera{camera.serial}/{srv_name}",
            lambda request, response: callback(camera, request, response),
        )

    def _call_stream_service_async(self, serial: str, srv_name: str) -> None:
        client = self.create_client(
            Empty, f"/camera_streamer/stream/camera{serial}/{srv_name}"
        )
        client.call_async(Empty.Request()).add_done_callback(
            lambda future: self.destroy_client(client)
        )

    def _register_camera(self, camera: Camera) -> None:
        self.get_logger().info(f"Registering camera {camera.serial}.")

        # Create stream control services for the camera.
        self._camera_registrations[camera.serial] = CameraRegistration(
            services=[
                self._create_stream_service(
                    camera, "start", self._stream_start_callback
                ),
                self._create_stream_service(
                    camera, "pause", self._stream_pause_callback
                ),
                self._create_stream_service(camera, "stop", self._stream_stop_callback),
            ]
        )

        # Start streaming the camera, asynchronously.
        self._call_stream_service_async(camera.serial, "start")

    def _unregister_camera(self, serial: str) -> None:
        self.get_logger().info(f"Unregistering camera {serial}.")

        # Stop streaming the camera, asynchronously.
        self._call_stream_service_async(serial, "stop")

        # Remove the camera's stream control services.
        for service in self._camera_registrations[serial].services:
            self.destroy_service(service)
        del self._camera_registrations[serial]

    def _register_new_cameras(self, cameras: Cameras) -> None:
        """
        Register new cameras, and remove old ones.
        :param cameras: The current list of cameras.
        """
        for camera in cameras.cameras:
            if camera.serial not in self._camera_registrations:
                self._register_camera(camera)

        current_serials = {camera.serial for camera in cameras.cameras}
        known_serials = set(self._camera_registrations.keys())
        for known_serial in known_serials:
            if known_serial not in current_serials:
                self._unregister_camera(known_serial)

    @staticmethod
    def _create_camera_bin(camera: Camera) -> Gst.Bin:
        # Create the bin and elements.
        gst_bin: Gst.Bin = Gst.Bin.new(f"camera-{camera.serial}-bin")

        source = Gst.ElementFactory.make("v4l2src", "source")
        # TODO: Cameras may not always support JPEG. Fall back to another format if this fails.
        # videoconvert is recommended here (when not using jpegdec) as v4l2src has unknown capabilities.
        # https://gstreamer.freedesktop.org/documentation/tutorials/basic/handy-elements.html#videoconvert
        # videoconvert = Gst.ElementFactory.make("videoconvert", "videoconvert")
        decoder = Gst.ElementFactory.make("jpegdec", "decoder")
        sink = Gst.ElementFactory.make("webrtcsink", "sink")

        # Configure the elements.
        # # Source
        source.props.device = camera.node

        # # Sink
        # ## WebRTC settings
        # TODO: Move to gcc (Google Congestion Control) when GStreamer 1.22 is released.
        sink.props.congestion_control = "homegrown"

        # ## Metadata
        meta: Gst.Structure = Gst.Structure.new_empty("meta")
        meta.set_value("serial", camera.serial)
        sink.props.meta = meta

        # Add the elements to the bin, and link them.
        elements = [source, decoder, sink]
        gst_bin.add(*elements)
        # noinspection PyUnresolvedReferences
        Gst.Element.link_many(*elements)

        return gst_bin

    def _stream_start_callback(
        self,
        camera: Camera,
        request: Empty.Request,
        response: Empty.Response,
    ) -> Empty.Response:
        gst_bin = self._gst_bins.get(camera.serial)
        if gst_bin is None:
            gst_bin = self._create_camera_bin(camera)
            self._gst_pipeline.add(gst_bin)
            self._gst_bins[camera.serial] = gst_bin

        self.get_logger().info(f"Starting stream for camera {camera.serial}.")
        gst_bin.set_state(Gst.State.PLAYING)

        return response

    def _stream_pause_callback(
        self,
        camera: Camera,
        request: Empty.Request,
        response: Empty.Response,
    ) -> Empty.Response:
        gst_bin = self._gst_bins.get(camera.serial)
        if gst_bin is None:
            return response

        self.get_logger().info(f"Pausing stream for camera {camera.serial}.")
        gst_bin.set_state(Gst.State.PAUSED)

        return response

    def _stream_stop_callback(
        self,
        camera: Camera,
        request: Empty.Request,
        response: Empty.Response,
    ) -> Empty.Response:
        gst_bin = self._gst_bins.pop(camera.serial, None)
        if gst_bin is None:
            return response

        self.get_logger().info(f"Stopping stream for camera {camera.serial}.")
        gst_bin.set_state(Gst.State.NULL)
        self._gst_pipeline.remove(gst_bin)

        return response


def main(args=None):
    rclpy.init(args=args)
    server = CameraStreamerService()
    rclpy.spin(server)
    server.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
