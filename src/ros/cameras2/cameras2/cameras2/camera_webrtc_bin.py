from typing import Optional

import gi

gi.require_version("Gst", "1.0")  # noqa
from gi.repository import Gst

from cameras2.utils import dict_to_gst_structure, gst_structure_to_dict


class CameraWebRTCBin:
    bin: Gst.Bin

    _source: Gst.Element
    _caps_filter: Gst.Element
    _video_converter: Gst.Element
    _clock_overlay: Gst.Element
    _sink: Gst.Element

    def __init__(
        self,
        serial: str,
        device_node: str,
        width: Optional[int] = None,
        height: Optional[int] = None,
        framerate: Optional[int] = None,
        do_fec: bool = True,
        do_retransmission: bool = True,
        show_clock: bool = True,
        extra_meta: Optional[dict[str, object]] = None,
    ):
        # Create and configure the elements.
        # # Source
        self._source = Gst.ElementFactory.make("v4l2src", "source")
        self._source.props.device = device_node

        # # Capability filter
        caps_structure = Gst.Structure.new_empty("video/x-raw")
        if width is not None:
            caps_structure.set_value("width", width)
        if height is not None:
            caps_structure.set_value("height", height)
        if framerate is not None:
            caps_structure.set_value("framerate", Gst.Fraction(framerate, 1))

        caps = Gst.Caps.new_empty()
        caps.append_structure(caps_structure)

        self._caps_filter = Gst.ElementFactory.make("capsfilter", "capsfilter")
        self._caps_filter.props.caps = caps

        # # Video converter
        self._video_converter = Gst.ElementFactory.make("videoconvert", "videoconvert")

        # # Clock overlay
        self._clock_overlay = Gst.ElementFactory.make("clockoverlay", "clockoverlay")

        # # Sink
        self._sink = Gst.ElementFactory.make("webrtcsink", "sink")
        # ## WebRTC settings
        self._sink.props.congestion_control = "gcc"
        self._sink.props.do_fec = do_fec
        self._sink.props.do_retransmission = do_retransmission
        self._sink.props.stun_server = None
        # ## Metadata
        self._sink.props.meta = dict_to_gst_structure(
            "meta",
            {"serial": serial, **(extra_meta if extra_meta is not None else {})},
        )

        # Create the bin, and add the elements.
        self.bin = Gst.Bin.new(f"camera-{serial}-bin")
        potential_elements = [
            self._source,
            self._caps_filter,
            self._video_converter,
            self._clock_overlay if show_clock else None,
            self._sink,
        ]
        elements = list(filter(lambda element: element is not None, potential_elements))
        self.bin.add(*elements)
        Gst.Element.link_many(*elements)

    @property
    def webrtc_stats(self) -> dict[str, object]:
        return gst_structure_to_dict(self._sink.props.stats)
