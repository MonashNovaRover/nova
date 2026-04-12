#include <string>
#include <gst/gst.h>
#include "properties/sinks.hpp"

void set_webrtcsink(GstElement* webrtc, const std::string serial, const std::string video_caps, const bool do_fec, const bool do_retransmission, const std::string congestion_control, const int bitrate) {
    GstStructure *meta = gst_structure_new("meta", "serial", G_TYPE_STRING, serial.c_str(), NULL); 
    GstCaps *webrtc_caps = gst_caps_from_string(video_caps.c_str());
    g_object_set(webrtc,
        "do-fec", do_fec,
        "do-retransmission", do_retransmission,
        "congestion-control", (
            congestion_control == "disabled" ? 0 :
            congestion_control == "homegrown" ? 1 :
            congestion_control == "gcc" ? 2 :
            2),
        "max-bitrate", bitrate*1125,
        "meta", meta,
        "video-caps", webrtc_caps,
        NULL);
    gst_caps_unref(webrtc_caps);
    gst_structure_free(meta);
}

