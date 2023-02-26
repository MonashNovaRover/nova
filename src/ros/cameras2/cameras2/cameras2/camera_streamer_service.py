from typing import Callable, Type

import gi
import rclpy
import rclpy.node
from std_srvs.srv import Empty

gi.require_version("Gst", "1.0")
from gi.repository import Gst

from camera_msgs.msg import Camera
from camera_msgs.srv import GetCameras


class CameraStreamerService(rclpy.node.Node):
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

        # Retrieve the list of cameras connected to the system.
        self.get_logger().info("Retrieving list of cameras...")
        directory_client = self.create_client(
            GetCameras, "/camera_directory/get_cameras"
        )
        while not directory_client.wait_for_service(1.0):
            self.get_logger().info("Waiting for camera directory service...")
        cameras_future = directory_client.call_async(GetCameras.Request())
        rclpy.spin_until_future_complete(self, cameras_future)
        cameras = cameras_future.result().cameras
        self.destroy_client(directory_client)

        for camera in cameras:
            self.register_camera(camera)

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
    ):
        self.create_service(
            srv_type,
            f"/camera_streamer/stream/camera{camera.serial}/{srv_name}",
            lambda request, response: callback(camera, request, response),
        )

    def register_camera(self, camera: Camera):
        self.get_logger().info(f"Registering camera {camera.serial}.")

        # Create stream control services for the camera.
        self._create_stream_service(camera, "start", self._stream_start_callback)
        self._create_stream_service(camera, "pause", self._stream_pause_callback)
        self._create_stream_service(camera, "stop", self._stream_stop_callback)

        # Start streaming the camera, asynchronously.
        start_client = self.create_client(
            Empty, f"/camera_streamer/stream/camera{camera.serial}/start"
        )
        start_client.call_async(Empty.Request()).add_done_callback(
            lambda future: self.destroy_client(start_client)
        )

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
