import functools
from typing import Optional

import gi

gi.require_version("Gst", "1.0")  # noqa
from gi.repository import Gst

from cameras2.utils import dict_to_gst_structure, gst_structure_to_dict


class CameraWebRTCBin:
    bin: Gst.Bin

    _source: Gst.Element
    _caps_filter: Gst.Element
    _decoder: Gst.Element
    _video_converter: Gst.Element
    _clock_overlay: Gst.Element | None
    _sink: Gst.Element

    def __init__(
        self,
        serial: str,
        device_node: str,
        # sink
        mime: str = "video/x-raw",
        width: Optional[int] = None,
        height: Optional[int] = None,
        framerate: Optional[int] = None,
        do_fec: bool = True,
        do_retransmission: bool = False, # Increases latency
        max_bitrate: int = 819200, # 0.8 megabit/s
        video_caps: str = "video/x-h264; video/x-vp9; video/x-h265",
        show_clock: bool = True,
        extra_meta: Optional[dict[str, object]] = None,
        # decoder
        low_percent: int = 1
    ):
        self.bin = Gst.Bin.new(f"camera-{serial}-bin")

        # Create and configure the elements.
        # # Sink
        self._sink = Gst.ElementFactory.make("webrtcsink", "sink")
        # ## WebRTC settings
        self._sink.props.congestion_control = "gcc"
        self._sink.props.do_fec = do_fec
        self._sink.props.do_retransmission = do_retransmission
        self._sink.props.stun_server = None
        self._sink.max_bitrate = max_bitrate
        self._sink.video_caps = video_caps
        # ## Metadata
        self._sink.props.meta = dict_to_gst_structure(
            "meta",
            {"serial": serial, **(extra_meta if extra_meta is not None else {})},
        )
        self.bin.add(self._sink)

        # # Clock overlay
        if show_clock:
            self._clock_overlay = Gst.ElementFactory.make(
                "clockoverlay", "clockoverlay"
            )
            self.bin.add(self._clock_overlay)
            self._clock_overlay.link(self._sink)
        else:
            self._clock_overlay = None

        # # Converter
        self._video_converter = Gst.ElementFactory.make("videoconvert", "converter")
        self.bin.add(self._video_converter)
        self._video_converter.link(
            self._clock_overlay if self._clock_overlay is not None else self._sink
        )

        # # Decoder
        self._decoder = Gst.ElementFactory.make("decodebin", "decoder")
        self._decoder.connect(
            "pad-added",
            lambda element, pad: pad.link(self._video_converter.get_static_pad("sink")),
        )
        # Lower buffering threshold
        self._decoder.props.low_percent = low_percent
        self.bin.add(self._decoder)

        # # Encoder # WIP, taken from bitmovin h264 config
        self._encoder = Gst.ElementFactory.make("x264enc", "encoder")
        self._encoder.props.b_adapt = False
        self._encoder.props.cabac = False
        self._encoder.props.key_int_max = 3 # GOP of 6 or latency of 400ms
        self._encoder.props.mb_tree = False
        self._encoder.props.me = "dia"
        self._encoder.props.quantizer = 40 # Lower default size but lower filesize as well
        self._encoder.props.rc_lookahead = 0
        self._encoder.props.ref = 1
        self._encoder.props.speed_preset = "ultrafast"
        self._encoder.props.threads = 1 # Limit thread locking
        self._encoder.props.trellis = False # Disable search quantization algorithm
        self._encoder.props.tune = "zerolatency"
        self._encoder.props.vbv_buf_capacity = 600 # Max with 10 fps and gop
        self.bin.add(self._encoder)

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
