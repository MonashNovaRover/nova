#ifndef CPUFILTERS_HEADER
#define CPUFILTERS_HEADER

#include <string>
#include <gst/gst.h>

int crop43(const int width, const int height);

void set_crop43(GstElement* cropper, const int crop_width, const int downscale);

void set_convertscale(GstElement* convert, const std::string chroma_resampler, const std::string dither, const std::string method);

#endif
