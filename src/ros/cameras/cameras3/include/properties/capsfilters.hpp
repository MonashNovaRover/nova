#ifndef CAPSFILTERS_HEADER
#define CAPSFILTERS_HEADER

#include <string>
#include <gst/gst.h>

void set_srcfilter(GstElement* filter, const std::string mime, const int width, const int height, const int framerate, const int framerate_denominator, const int downrate, const int brightness, const int contrast);

void set_scalefilter(GstElement* filter, const std::string format, const int width, const int height, const int framerate, const int framerate_denominator, const int downscale, const int downrate, const int brightness, const int contrast);

#endif
