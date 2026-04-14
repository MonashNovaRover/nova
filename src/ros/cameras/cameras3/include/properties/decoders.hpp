#ifndef DECODER_HEADER
#define DECODER_HEADER

#include <string>
#include <gst/gst.h>

void set_jpegdec(GstElement* decode, const std::string jpegdec_method);

#endif
