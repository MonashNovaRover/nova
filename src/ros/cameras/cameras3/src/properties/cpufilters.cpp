#include <string>
#include <gst/gst.h>
#include "properties/cpufilters.hpp"

int crop43(const int width, const int height) {
  return (width-(height*4/3))/2;
}

void set_queue(GstElement* element) {
  g_object_set(element,
    "max-size-buffers", 1,
    "max-size-time", 0,
    "max-size-bytes", 0,
    "leaky", 2, // Drop old buffers
  NULL);
};

void set_cpu_crop43(GstElement* element) {
  gst_util_set_object_arg(G_OBJECT(element), "aspect-ratio", "4/3");
};

void set_no_cpu_crop43(GstElement* element) {
  gst_util_set_object_arg(G_OBJECT(element), "aspect-ratio", "0/1");
};
