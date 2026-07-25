#include <gst/gst.h>
#include "properties/glfilters.hpp"

void set_no_glcrop43(GstElement* element) { 
  g_object_set(element,
    "scale-x", 1.0,
  NULL);
};
