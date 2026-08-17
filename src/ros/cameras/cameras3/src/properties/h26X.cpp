#include <string>
#include <gst/gst.h>
#include "properties/h26X.hpp"

void set_h264payload(GstElement* element) {
  // Apply patch for gc2093
  g_object_set(element,
      "aggregate-mode", 1,
      "config-interval", -1,
      NULL);
}

void set_h264parse(GstElement* element, const int interval) {
  g_object_set(element,
    "config-interval", interval,
    //"disable-passthrough", true,
  NULL);
}

void set_h265parse(GstElement* element, const int interval) {
  g_object_set(element,
    "config-interval", interval,
    //"disable-passthrough", true,
  NULL);
}
