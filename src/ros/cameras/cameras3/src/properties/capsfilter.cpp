#include <string>
#include <gst/gst.h>
#include "properties/capsfilters.hpp"

void set_no_cpu_grey_filter(GstElement* element) {
  g_object_set(element, "caps", NULL, NULL);
}
