#include <string>
#include <gst/gst.h>
#include "properties/glfilters.hpp"

void set_gledgedetect(GstElement* gledgedetect) { 
  g_object_set(gledgedetect,
    "effect", 16, // Sobel
  NULL);
};

