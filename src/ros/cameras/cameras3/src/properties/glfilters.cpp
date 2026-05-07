#include <gst/gst.h>
#include "properties/glfilters.hpp"

void set_glgreyscale(GstElement* element) { 
  g_object_set(element,
    "saturation", 0.0, // turn off colour
  NULL);
};

void set_no_glgreyscale(GstElement* element) { 
  g_object_set(element,
    "saturation", 1.0, // turn on colour
  NULL);
};

void set_no_glcrop43(GstElement* element) { 
  g_object_set(element,
    "scale-x", 1.0,
  NULL);
};
