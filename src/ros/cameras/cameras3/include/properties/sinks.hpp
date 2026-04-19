#ifndef SINKS_HEADER
#define SINKS_HEADER

#include <string>
#include <gst/gst.h>

void set_webrtcsink(GstElement* webrtc, const std::string serial, const std::string video_caps, const bool do_fec, const bool do_retransmission, const std::string congestion_control, const int bitrate);

#endif
