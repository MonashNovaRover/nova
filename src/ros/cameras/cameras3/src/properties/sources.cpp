#include <gst/gst.h>

void set_thetasource(GstElement* element) {
  g_object_set(element,
    "mode", 0, // Only use 2k. 4k is mode 1
    NULL);
}
