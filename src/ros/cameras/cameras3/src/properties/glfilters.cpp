#include <string>
#include <gst/gst.h>
#include "properties/glfilters.hpp"

void set_glgreyscale(GstElement* glgreyscale) { 
  g_object_set(glgreyscale,
    "saturation", 0.0, // Turn off colour
  NULL);
};

