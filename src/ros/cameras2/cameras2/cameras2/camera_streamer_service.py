from typing import Callable, cast, NamedTuple
import json

import gi
import rclpy
from rclpy import Future, qos, Parameter
from rclpy.logging import LoggingSeverity
from rclpy.node import Node
from rclpy.service import Service
from rclpy.client import Client

from rcl_interfaces.msg import ParameterDescriptor, ParameterType

gi.require_version("Gst", "1.0")  # noqa
from gi.repository import Gst, GLib

from camera_msgs.msg import Cameras
from camera_msgs.srv import CameraOperation, GetCameraStreamStats

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
      - /camera_streamer/stream/start [camera_msgs/CameraOperation]
      - /camera_streamer/stream/pause [camera_msgs/CameraOperation]
      - /camera_streamer/stream/stop [camera_msgs/CameraOperation]
      - /camera_streamer/stream/get_stats [camera_msgs/GetCameraStreamStats]
    ACTIONS: None
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    PACKAGE: 	cameras2
    AUTHOR(S):	Joshua Leivenzon
    CREATION:	25/02/2023
    EDITED:		25/02/2023
    ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
    """

    class CameraConfiguration(NamedTuple):
        width: int
        height: int
        framerate: int
        show_clock: bool
        meta: dict[str, object]

    def __init__(self):
        super().__init__(
            "camera_streamer",
            allow_undeclared_parameters=True,
            automatically_declare_parameters_from_overrides=True,
        )

        # Declare general configuration options.
        for name in {"autostart"}:
            if self.has_parameter(name):
                self.undeclare_parameter(name)
        self.declare_parameter(
            "autostart",
            False,
            ParameterDescriptor(
                type=ParameterType.PARAMETER_BOOL,
                description="Automatically start streaming cameras when they are connected.",
            ),
        )

        # Load the camera configuration parameters.
        self._default_camera_configuration, self._camera_configurations = self._load_camera_configurations()

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
        self.create_service(GetCameraStreamStats, "/camera_streamer/stream/get_stats", self._stats_callback)

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

    def _load_camera_configurations(self) -> tuple[CameraConfiguration, dict[str, CameraConfiguration]]:
        def expand_dictionary(dictionary: dict[str, object]) -> dict[str, object]:
            output = {}
            for key, value in dictionary.items():
                keys = key.split(".")
                current = output
                for subkey in keys[:-1]:
                    current = current.setdefault(subkey, {})
                current[keys[-1]] = value
            return output

        def read_meta(parameters: dict[str, object]) -> dict[str, object]:
            return {
                name: read_meta(value) if isinstance(value, dict) else cast(Parameter, value).value
                for name, value in parameters.items()
            }

        def read_camera_configuration(
            defaults: CameraStreamerService.CameraConfiguration,
            parameters: dict[str, object],
        ) -> CameraStreamerService.CameraConfiguration:
            def get_parameter_value(name: str, read: Callable[[Parameter], object]):
                return read(cast(Parameter, parameters[name]).get_parameter_value()) if name in parameters else None

            width = get_parameter_value("width", lambda p: p.integer_value)
            height = get_parameter_value("height", lambda p: p.integer_value)
            framerate = get_parameter_value("framerate", lambda p: p.integer_value)
            show_clock = get_parameter_value("show_clock", lambda p: p.bool_value)

            return CameraStreamerService.CameraConfiguration(
                width=width if width is not None else defaults.width,
                height=height if height is not None else defaults.height,
                framerate=framerate if framerate is not None else defaults.framerate,
                show_clock=show_clock if show_clock is not None else defaults.show_clock,
                meta={**defaults.meta, **read_meta(parameters.get("meta", {}))},
            )

        param_defaults = read_camera_configuration(
            CameraStreamerService.CameraConfiguration(
                width=0,
                height=0,
                framerate=0,
                show_clock=True,
                meta={},
            ),
            expand_dictionary(self.get_parameters_by_prefix("defaults")),
        )

        return param_defaults, {
            serial: read_camera_configuration(param_defaults, cast(dict[str, object], camera_parameters))
            for serial, camera_parameters in expand_dictionary(self.get_parameters_by_prefix("cameras")).items()
        }

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

        if self.get_parameter("autostart").get_parameter_value().bool_value:
            self._stream_start_client.call_async(CameraOperation.Request(serials=set(kwargs.keys())))

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

    def _create_camera_bin(self, serial: str, device_node: str) -> CameraWebRTCBin:
        camera_configuration = self._camera_configurations.get(serial, self._default_camera_configuration)
        camera_bin = CameraWebRTCBin(
            serial,
            device_node,
            width=camera_configuration.width if camera_configuration.width != 0 else None,
            height=camera_configuration.height if camera_configuration.height != 0 else None,
            framerate=camera_configuration.framerate if camera_configuration.framerate != 0 else None,
            show_clock=camera_configuration.show_clock,
            extra_meta=camera_configuration.meta,
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

    def _stats_callback(
        self,
        request: GetCameraStreamStats.Request,
        response: GetCameraStreamStats.Response,
    ) -> GetCameraStreamStats.Response:
        result = {
            serial: self._camera_bins[serial].webrtc_stats
            for serial in (request.serials if request.serials else self._camera_bins.keys())
            if serial in self._camera_bins
        }
        response.result_json = json.dumps(result, indent=None if request.indent == 0 else request.indent)
        return response


def main(args=None):
    rclpy.init(args=args)
    server = CameraStreamerService()
    rclpy.spin(server)
    server.destroy_node()
    rclpy.shutdown()


if __name__ == "__main__":
    main()
