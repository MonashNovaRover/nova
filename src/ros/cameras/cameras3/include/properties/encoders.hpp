#ifndef ENCODER_HEADER
#define ENCODER_HEADER

#include <string>
#include <gst/gst.h>

void set_av1enc(GstElement* encode, const int cpu_used, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate);

void set_vp8enc(GstElement* encode, const int cpu_used, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate);

void set_vp9enc(GstElement* encode, const int cpu_used, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate);

void set_x264enc(GstElement* encode, const int cpu_used, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate);

#endif
