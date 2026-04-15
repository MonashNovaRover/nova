#ifndef H264_HEADER
#define H264_HEADER

#include <string>
#include <gst/gst.h>

void set_h264payload(GstElement* payload);

void set_h264parse(GstElement* parse, const int interval = -1);

#endif

