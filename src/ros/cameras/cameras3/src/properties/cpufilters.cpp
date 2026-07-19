#include <cstddef>
#include <string>
#include <gst/gst.h>
#include "properties/cpufilters.hpp"

int crop43(const int width, const int height, const float zoom) {
  return (int)((float)((width-(height*4/3))/2)/zoom);
}

void set_queue(GstElement* element) {
  g_object_set(element,
    "max-size-buffers", 1,
    "max-size-time", 1,
    "max-size-bytes", 1,
    "leaky", 2, // Drop old buffers
    "silent", true,
  NULL);
};
