#ifndef VPX_HEADER
#define VPX_HEADER

#include <string>
#include <gst/gst.h>

void set_vpXenc(GstElement* encode, const int deadline, const int cpu_used, const std::string end_usage, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate, const std::string video_caps, const std::string aq_mode);

#endif
