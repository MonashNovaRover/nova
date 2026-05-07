#ifndef CAPSFILTERS_HEADER
#define CAPSFILTERS_HEADER

#include <string>
#include <algorithm>
#include <gst/gst.h>


template<typename properties> void set_srcfilter(GstElement* element, const properties& props) {
  GstCaps *caps = gst_caps_new_simple(
    props->mime.c_str(),
    "width", G_TYPE_INT, props->width,
    "height", G_TYPE_INT, props->height,
    "framerate", GST_TYPE_FRACTION, props->framerate, props->framerate_denominator*props->downrate,
    "brightness", G_TYPE_INT, std::clamp(props->brightness, 0, 255),
    "contrast", G_TYPE_INT,  std::clamp(props->contrast, 0, 255),
  NULL);
  g_object_set(element, "caps", caps, NULL);
  gst_caps_unref(caps);
}

template<typename properties> void set_scalefilter(GstElement* element, const properties& props) {
  const std::string mime = "video/x-raw";
  GstCaps *caps = gst_caps_new_simple(
    mime.c_str(),
    "format", G_TYPE_STRING, props->format.c_str(),
    "width", G_TYPE_INT, (int) ((float) props->width/ (float) props->downscale),
    "height", G_TYPE_INT, (int) ((float) props->height/ (float) props->downscale),
  NULL);
  g_object_set(element, "caps", caps, NULL);
  gst_caps_unref(caps);
}

template<typename properties> void set_scalefilter(GstElement* element, const properties& props, const int crop_width) {
  const std::string mime = "video/x-raw";
  GstCaps *caps = gst_caps_new_simple(
    mime.c_str(),
    "format", G_TYPE_STRING, props->format.c_str(),
    "width", G_TYPE_INT, (int) ((float) (props->width-crop_width)/ (float) props->downscale),
    "height", G_TYPE_INT, (int) ((float) props->height/ (float) props->downscale),
  NULL);
  g_object_set(element, "caps", caps, NULL);
  gst_caps_unref(caps);
}

template<typename properties> void set_rosfilter(GstElement* element, const properties& props) {
  const std::string mime = "video/x-raw";
  GstCaps *caps = gst_caps_new_simple(
    mime.c_str(),
    "format", G_TYPE_STRING, props->ros_format.c_str(),
  NULL);
  g_object_set(element, "caps", caps, NULL);
  gst_caps_unref(caps);
}

void set_cpu_grey_filter(GstElement* element);
void set_no_cpu_grey_filter(GstElement* element); 

#endif
