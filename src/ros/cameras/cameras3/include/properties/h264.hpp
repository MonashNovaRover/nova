#ifndef H264_HEADER
#define H264_HEADER

#include <string>
#include <gst/gst.h>

void set_x264enc(GstElement* encode, const std::string tune, const std::string speed_preset, const int threads, const int bitrate, const int noise_reduction, const int gop, const int framerate, const int framerate_denominator, const int downrate);

void set_h264payload(GstElement* payload, const bool payload_quirk);

void set_h264parse(GstElement* parse, const int interval = -1);

#endif
