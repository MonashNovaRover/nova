#include <gst/gst.h>
#include "properties/glfilters.hpp"

void set_glgreyscale(GstElement* glgreyscale) { 
  g_object_set(glgreyscale,
    "saturation", 0.0, // turn off colour
  NULL);
};

void set_no_glgreyscale(GstElement* glgreyscale) { 
  g_object_set(glgreyscale,
    "saturation", 1.0, // turn on colour
  NULL);
};

