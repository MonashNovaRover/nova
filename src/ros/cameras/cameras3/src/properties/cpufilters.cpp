#include <string>
#include <gst/gst.h>
#include "properties/cpufilters.hpp"

int crop43(const int width, const int height) {
  return (width-(height*4/3))/2;
}

void set_crop43(GstElement* cropper, const bool crop43, const int crop_width, const int downscale) {
    if (crop43) {
        g_object_set(cropper,
          "left", crop_width/downscale,
          "right", crop_width/downscale,
          NULL);
    }
}

void set_convertscale(GstElement* convert, const std::string chroma_resampler, const std::string dither, const std::string method) {
  g_object_set(convert,
      "chroma-resampler", (
          chroma_resampler == "nearest" ? 0 :
          chroma_resampler == "linear" ? 1 :
          chroma_resampler == "cubic" ? 2 :
          chroma_resampler == "sinc" ? 3 : 
          chroma_resampler == "lanczos" ? 4 :
          0),
      "dither", (
          dither == "none" ? 0 :
          dither == "verterr" ? 1 :
          dither == "floyd-steinberg" ? 2 :
          dither == "sierra-lite" ? 3 : 
          dither == "bayer" ? 4 :
          4),
      "method", (
          method == "nearest-neighbour" ? 0 :
          method == "bilinear" ? 1 :
          method == "4-tap" ? 2 :
          method == "lanczos" ? 3 : 
          method == "bilinear2" ? 4 :
          method == "sinc" ? 5 :
          method == "hermite" ? 6 :
          method == "spline" ? 7 :
          method == "catrom" ? 8 : 
          method == "mitchell" ? 9 :
          0),
      NULL);
}
