#include <cstddef>
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
    "silent", true,
  NULL);
};

void set_no_cpu_crop43(GstElement* element) {
  g_object_set(element,
    "left", 0,
    "right", 0,
  NULL);
};
