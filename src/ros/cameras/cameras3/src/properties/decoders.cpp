#include <string>
#include <gst/gst.h>
#include "properties/decoders.hpp"

void set_jpegdec(GstElement* encode, const std::string jpegdec_method) {
    g_object_set(encode,
        "idct-method", (
          jpegdec_method == "ifast" ? 1:
          jpegdec_method == "islow" ? 0:
          jpegdec_method == "float" ? 2:
          1 ), // ifast is default, but float is used for max quality 
        NULL);
}

