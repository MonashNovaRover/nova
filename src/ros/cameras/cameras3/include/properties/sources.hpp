#ifndef SOURCES_HEADER
#define SOURCES_HEADER

#include <string>
#include <gst/gst.h>

void set_v4lsource(GstElement* source, const std::string device, const std::string io_mode);

#endif
