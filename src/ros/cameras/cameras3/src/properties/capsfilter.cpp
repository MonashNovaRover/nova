#include <string>
#include <gst/gst.h>
#include "properties/capsfilters.hpp"

void set_greyfilter(GstElement* filter) {
  const std::string mime = "video/x-raw", format = "GRAY8";
  GstCaps *caps = gst_caps_new_simple(
    mime.c_str(),
    "format", G_TYPE_STRING, format.c_str(),
  NULL);
  g_object_set(filter, "caps", caps, NULL);
  gst_caps_unref(caps);
}

void set_no_greyfilter(GstElement* filter) {
  g_object_set(filter, "caps", NULL, NULL);
}
