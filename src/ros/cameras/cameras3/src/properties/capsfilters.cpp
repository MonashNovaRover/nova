#include <string>
#include <gst/gst.h>
#include "properties/capsfilters.hpp"

void set_srcfilter(GstElement* filter, const std::string mime, const int width, const int height, const int framerate, const int framerate_denominator, const int downrate, const int brightness, const int contrast) {
  GstCaps *caps = gst_caps_new_simple(
      mime.c_str(),
      "width", G_TYPE_INT, width,
      "height", G_TYPE_INT, height,
      "framerate", GST_TYPE_FRACTION, framerate, framerate_denominator*downrate,
      "brightness", G_TYPE_INT, brightness,
      "contrast", G_TYPE_INT,  contrast,
      NULL);
  g_object_set(filter, "caps", caps, NULL);
  gst_caps_unref(caps);
}

void set_scalefilter(GstElement* filter, const std::string format, const int width, const int height, const int framerate, const int framerate_denominator, const int downscale, const int downrate, const int brightness, const int contrast) {
  const std::string mime = "video/x-raw";
  GstCaps *caps = gst_caps_new_simple(
      mime.c_str(),
      "format", G_TYPE_STRING, format.c_str(),
      "width", G_TYPE_INT, (int) ((float) width/ (float) downscale),
      "height", G_TYPE_INT, (int) ((float) height/ (float) downscale),
      "framerate", GST_TYPE_FRACTION, framerate, framerate_denominator*downrate,
      "brightness", G_TYPE_INT, brightness,
      "contrast", G_TYPE_INT,  contrast,
      NULL);
  g_object_set(filter, "caps", caps, NULL);
  gst_caps_unref(caps);
}

