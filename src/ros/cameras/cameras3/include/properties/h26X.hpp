#ifndef H264_HEADER
#define H264_HEADER

#include <gst/gst.h>

void set_h264payload(GstElement* element);
void set_h264parse(GstElement* element, const int interval);
void set_h265parse(GstElement* element, const int interval);

#endif

