#ifndef AV1_HEADER
#define AV1_HEADER

#include <string>
#include <gst/gst.h>

void set_av1enc(GstElement* encode, const int cpu_used, const std::string end_usage, const std::string usage_profile, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate);

#endif
