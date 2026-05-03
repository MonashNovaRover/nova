#ifndef DECODER_HEADER
#define DECODER_HEADER

#include <string>
#include <gst/gst.h>

template<typename properties> void set_jpegdec(GstElement* decode, const properties& props) {
  g_object_set(decode,
    "idct-method", (
      props->jpegdec_method == "ifast" ? 1:
      props->jpegdec_method == "islow" ? 0:
      props->jpegdec_method == "float" ? 2:
      1 ), // ifast is default, but float is used for max quality 
  NULL);
}

#endif
