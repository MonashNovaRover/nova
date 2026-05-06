#include <string>
#include <gst/gst.h>
#include "properties/cpufilters.hpp"

int crop43(const int width, const int height) {
  return (width-(height*4/3))/2;
}

void set_queue(GstElement* queue) {
  g_object_set(queue,
    "leaky", 2, // Drop old buffers
  NULL);
};

void set_no_crop43(GstElement* cropper) {
  g_object_set(cropper,
    "left", 0,
    "right", 0,
  NULL);
};
