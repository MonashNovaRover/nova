"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

An amalgmation of Josh's beautiful code.
This service manages a GStreamer pipeline to
stream video footage from ros Image topics over WebRTC.
Consult the repository README for complete setup
instructions.
This node can only be run when the camera stack is not being run
    e.g for gazebo sim only

There is also gstreamer pipeline classes at the end.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: camera_streamer
TOPICS: None
SERVICES:
    - /camera_streamer/stream/start [camera_msgs/CameraOperation]
    - /camera_streamer/stream/pause [camera_msgs/CameraOperation]
    - /camera_streamer/stream/stop [camera_msgs/CameraOperation]
    - /camera_streamer/stream/get_stats [camera_msgs/GetCameraStreamStats]
ACTIONS: None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	cameras2
AUTHOR(S):	Anthony Lew
CREATION:	27/04/2025
EDITED:		27/04/2025
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""

from __future__ import annotations

import typing
from socket import AddressFamily
from typing import Callable, cast, NamedTuple, Optional
import json
import functools

import gi
import psutil
import rclpy
from rclpy import Future, qos, Parameter
from rclpy.logging import LoggingSeverity
from rclpy.node import Node
from rclpy.service import Service
from rclpy.client import Client

from rcl_interfaces.msg import ParameterDescriptor, ParameterType

gi.require_version("Gst", "1.0")  # noqa
from gi.repository import Gst, GLib

from camera_msgs.msg import Cameras, Camera
from camera_msgs.srv import CameraOperation, GetCameraStreamStats, GetIPList
from cameras2.utils import dict_to_gst_structure, gst_structure_to_dict

from sensor_msgs.msg import Image


class CameraStreamerService(Node):
    """
    This node is 
    """
    class CameraConfiguration():
        def __init__(self, serial: str, topic: str):
            self.serial = serial
            self.camera_bin = None
            self.topic = topic

    def __init__(self):
        super().__init__("ros_streamer")

        # Load the camera configuration parameters.
        self.cameras = {}
        cameras = self.declare_parameter('cameras', '').get_parameter_value().string_value

        if cameras == '':
            self.get_logger().fatal("No cameras specified.")
            self._kill()

        for camera in cameras.split(' '):
            serial = self.declare_parameter(f"{camera}.serial", camera).get_parameter_value().string_value
            topic = self.declare_parameter(f"{camera}.topic", f"/{camera}/image_raw").get_parameter_value().string_value
            self.cameras[serial] = self.CameraConfiguration(
                serial,
                topic
            )
            self.get_logger().info(f"[{camera}]:{serial} found with topic of '{topic}'")
            
        # Initialize GStreamer.
        self.get_logger().info("Initializing GStreamer...")
        # noinspection PyArgumentList
        Gst.init(None)
        self._gst_pipeline: Gst.Pipeline = Gst.Pipeline.new("ros-topic-pipeline")
        self._gst_pipeline.set_state(Gst.State.PLAYING)
        self._gst_pipeline.get_bus().set_sync_handler(self._handle_gst_message, None)

        # Create services and clients.
        self.get_logger().info("Creating stream control services...")
        self._create_stream_service("start", self._stream_start)
        self._create_stream_service("pause", self._stream_pause)
        self._create_stream_service("stop", self._stream_stop)
        self.create_service(GetCameraStreamStats, "/camera_streamer/stream/get_stats", self._stats_callback)

        # Create subscriptions and publishers
        cameras_publisher = self.create_publisher(
            Cameras,
            "/camera_directory/cameras",
            qos.QoSProfile(
                history=qos.HistoryPolicy.KEEP_LAST,
                depth=1,
                reliability=qos.ReliabilityPolicy.RELIABLE,
                durability=qos.DurabilityPolicy.TRANSIENT_LOCAL,
            ),
        )
        self._stream_start([serial for serial in self.cameras.keys()])
        cameras_publisher.publish(Cameras(cameras=[Camera(serial=serial, node=self.cameras[serial].topic) for serial in self.cameras.keys()]))

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

    def _create_stream_service(self, srv_name: str, callback: Callable[[set[str]], bool]) -> Service:
        def srv_callback(request: CameraOperation.Request, response: CameraOperation.Response) -> CameraOperation.Response:
            response.success = callback(
                set(request.serials).intersection(self.cameras.keys())
                if request.serials
                else set(self.cameras.keys())
            )
            self.get_logger().info(f"Recieved request for {srv_name}:{request.serials}")
            return response
        return self.create_service(CameraOperation, f"/camera_streamer/stream/{srv_name}", srv_callback)

    def _create_camera_bin(self, serial: str) -> RosCameraBin:
        camera_bin = RosCameraBin(serial, self.cameras[serial].topic)
        self.cameras[serial].camera_bin = camera_bin
        self._gst_pipeline.add(camera_bin.bin)
        return camera_bin

    def _stream_start(self, serials: set[str]) -> bool:
        for serial in serials:
            camera_bin = self.cameras[serial].camera_bin
            if camera_bin is None:
                self.get_logger().info(f"Starting stream for camera {serial}, topic: {self.cameras[serial].topic}.")
                camera_bin = self._create_camera_bin(serial)
            else:
                self.get_logger().info(f"Resuming stream for camera {serial}.")

            camera_bin.bin.set_state(Gst.State.PLAYING)
        return True

    def _stream_pause(self, serials: set[str]) -> bool:
        success = bool(serials)
        for serial in serials:
            camera_bin = self.cameras[serial].camera_bin
            if camera_bin is None:
                success = False
                continue

            self.get_logger().info(f"Pausing stream for camera {serial}.")
            camera_bin.bin.set_state(Gst.State.PAUSED)
        return success

    def _stream_stop(self, serials: set[str]) -> bool:
        success = bool(serials)
        for serial in serials:
            camera_bin = self.cameras[serial].camera_bin
            if camera_bin is None:
                success = False
                continue

            self.get_logger().info(f"Stopping stream for camera {serial}.")
            camera_bin.bin.set_state(Gst.State.NULL)
            self._gst_pipeline.remove(camera_bin.bin)
            self.cameras[serial].camera_bin = None
        return success

    def _stats_callback(self, request: GetCameraStreamStats.Request, response: GetCameraStreamStats.Response) -> GetCameraStreamStats.Response:
        result = {
            serial: self.cameras[serial].camera_bin.webrtc_stats
            for serial in (request.serials if request.serials else self.cameras.keys())
            if serial in self.cameras
        }
        response.result_json = json.dumps(result, indent=None if request.indent == 0 else request.indent)
        return response

    def _kill(self):
        self.destroy_node()
        rclpy.shutdown()


class RosCameraBin:
    """
    This gstreamer pipeline takes in a ros image topic and outputs a webrtc stream
    **Cannot** be used with camera_streamer_service.py
    """
    bin: Gst.Bin
    # rosimagesrc ! capsfilter ! decoder ! videoconvert ! webrtcsink

    def __init__(self, serial: str, topic: str):
        self.bin = Gst.Bin.new(f"camera-{serial}-bin")

        # Create and configure the elements.
        # # Sink
        self._sink = Gst.ElementFactory.make("webrtcsink", "sink")
        # ## WebRTC settings
        self._sink.props.congestion_control = "gcc"
        self._sink.props.do_fec = True
        self._sink.props.do_retransmission = True
        self._sink.props.stun_server = None
        # ## Metadata
        self._sink.props.meta = dict_to_gst_structure(
            "meta",
            {"serial": serial},
        )
        self.bin.add(self._sink)

        # # Converter
        self._video_converter = Gst.ElementFactory.make("videoconvert", "converter")
        self.bin.add(self._video_converter)
        self._video_converter.link(self._sink)


        self._queue = Gst.ElementFactory.make("queue", "queuer")
        self.bin.add(self._queue)
        self._queue.link(self._video_converter)

        # # Source
        self._source = Gst.ElementFactory.make("rosimagesrc", "source")
        self._source.set_property("ros-topic", topic)
        self.bin.add(self._source)
        self._source.link(self._queue)

    @property
    def webrtc_stats(self) -> dict[str, object]:
        return gst_structure_to_dict(self._sink.props.stats)

class CameraSplitROSWebRTCBin:
    """
    This gstreamer pipeline takes in a camera device in v4l2 and outputs both a webrtc stream and ros image topic
    Used with camera_streamer_service.py
    """
    bin: Gst.Bin
    # v4l2src ! capsfilter ! decoder ! videoconvert ! tee \
    # \ tee ! webrtcsink
    # \ tee ! rosimagesink
    def __init__(
        self,
        serial: str,
        device_node: str,
        mime: str = "video/x-raw",
        width: Optional[int] = None,
        height: Optional[int] = None,
        framerate: Optional[int] = None,
        do_fec: bool = True,
        do_retransmission: bool = True,
        show_clock: bool = True,
        extra_meta: Optional[dict[str, object]] = None,
    ):
        ros_topic = extra_meta['ros_topic']
        self.bin = Gst.Bin.new(f"camera-{serial}-bin")

        # Create and configure the elements.

        # # Web sink branch
        self._websink = Gst.ElementFactory.make("webrtcsink", "websink")
        # ## WebRTC settings
        self._websink.props.congestion_control = "gcc"
        self._websink.props.do_fec = do_fec
        self._websink.props.do_retransmission = do_retransmission
        self._websink.props.stun_server = None
        # ## Metadata
        self._websink.props.meta = dict_to_gst_structure(
            "meta",
            {"serial": serial, **(extra_meta if extra_meta is not None else {})},
        )
        self.bin.add(self._websink)

        # # Clock overlay
        if show_clock:
            self._clock_overlay = Gst.ElementFactory.make(
                "clockoverlay", "clockoverlay"
            )
            self.bin.add(self._clock_overlay)
            self._clock_overlay.link(self._websink)
        else:
            self._clock_overlay = None

        self._queue2 = Gst.ElementFactory.make("queue", "q2")
        self.bin.add(self._queue2)
        self._queue2.link(
            self._clock_overlay if self._clock_overlay is not None else self._websink
        )

        # # Ros Sink branch
        self._rossink = Gst.ElementFactory.make("rosimagesink", "rossink")
        self._rossink.set_property("ros-topic", ros_topic)
        self.bin.add(self._rossink)

        self._queue1 = Gst.ElementFactory.make("queue", "q1")
        self.bin.add(self._queue1)
        self._queue1.link(self._rossink)

        # # Tee split
        self._tee = Gst.ElementFactory.make("tee", "tee")
        self.bin.add(self._tee)
        self._tee.link(self._queue2)
        self._tee.link(self._queue1)

        # # Converter
        self._video_converter = Gst.ElementFactory.make("videoconvert", "converter")
        self.bin.add(self._video_converter)
        self._video_converter.link(self._tee)

        # # Decoder
        self._decoder = Gst.ElementFactory.make("decodebin", "decoder")
        self._decoder.connect(
            "pad-added",
            lambda element, pad: pad.link(self._video_converter.get_static_pad("sink")),
        )
        self.bin.add(self._decoder)

        # # Capability filter
        caps = Gst.Caps.new_empty()
        caps_structure = Gst.Structure.new_empty(mime)
        if width is not None:
            caps_structure.set_value("width", width)
        if height is not None:
            caps_structure.set_value("height", height)
        if framerate is not None:
            caps_structure.set_value("framerate", Gst.Fraction(framerate, 1))
        caps.append_structure(caps_structure)

        self._caps_filter = Gst.ElementFactory.make("capsfilter", "capsfilter")
        self._caps_filter.props.caps = caps
        self.bin.add(self._caps_filter)
        self._caps_filter.link(self._decoder)

        # # Source
        self._source = Gst.ElementFactory.make("v4l2src", "source")
        self._source.props.device = device_node
        self.bin.add(self._source)
        self._source.link(self._caps_filter)

    @property
    def webrtc_stats(self) -> dict[str, object]:
        return gst_structure_to_dict(self._sink.props.stats)



def main(args=None):
    rclpy.init(args=args)
    server = CameraStreamerService()
    rclpy.spin(server)
    server.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
