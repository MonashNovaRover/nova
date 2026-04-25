#include <string>
#include <gst/gst.h>
#include "properties/glfilters.hpp"

void set_glgreyscale(GstElement* glbalance) { 
  g_object_set(glbalance,
    "saturation", 0.0,
  NULL);
};

