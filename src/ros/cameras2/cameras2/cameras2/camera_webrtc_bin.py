from typing import Optional

import gi

gi.require_version("Gst", "1.0")  # noqa
from gi.repository import Gst


class CameraWebRTCBin:
    bin: Gst.Bin

    _source: Gst.Element
    _caps_filter: Gst.Element
    _video_converter: Gst.Element
    _sink: Gst.Element

    def __init__(
        self,
        serial: str,
        device_node: str,
        width: Optional[int] = None,
        fps: Optional[int] = None,
    ):
        # Create and configure the elements.
        # # Source
        self._source = Gst.ElementFactory.make("v4l2src", "source")
        self._source.props.device = device_node

        # # Capability filter
        caps_structure = Gst.Structure.new_empty("video/x-raw")
        if width is not None:
            caps_structure.set_value("width", width)
        if fps is not None:
            caps_structure.set_value("framerate", Gst.Fraction(fps, 1))

        caps = Gst.Caps.new_empty()
        caps.append_structure(caps_structure)

        self._caps_filter = Gst.ElementFactory.make("capsfilter", "capsfilter")
        self._caps_filter.props.caps = caps

        # # Video converter
        self._video_converter = Gst.ElementFactory.make("videoconvert", "videoconvert")

        # # Sink
        self._sink = Gst.ElementFactory.make("webrtcsink", "sink")
        # ## WebRTC settings
        self._sink.props.congestion_control = "gcc"
        self._sink.props.do_fec = True
        self._sink.props.do_retransmission = True
        # ## Metadata
        meta = Gst.Structure.new_empty("meta")
        meta.set_value("serial", serial)
        self._sink.props.meta = meta

        # Create the bin, and add the elements.
        self.bin = Gst.Bin.new(f"camera-{serial}-bin")
        elements = [
            self._source,
            self._caps_filter,
            self._video_converter,
            self._sink,
        ]
        self.bin.add(*elements)
        Gst.Element.link_many(*elements)
