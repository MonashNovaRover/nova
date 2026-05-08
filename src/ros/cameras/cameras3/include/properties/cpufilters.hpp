#ifndef CPUFILTERS_HEADER
#define CPUFILTERS_HEADER

#include <string>
#include <gst/gst.h>

int crop43(const int width, const int height);

void set_queue(GstElement* queue);

template<typename properties> void set_cpu_crop43(GstElement* element, const properties& props) {
  const int crop_width = crop43(props->width, props->height);
  g_object_set(element,
    "left", crop_width,
    "right", crop_width,
  NULL);
};

void set_no_cpu_crop43(GstElement* element);

template<typename properties> void set_convertscale(GstElement* element, const properties& props) {
  g_object_set(element,
    "chroma-resampler", (
      props->chroma_resampler == "nearest" ? 0 :
      props->chroma_resampler == "linear" ? 1 :
      props->chroma_resampler == "cubic" ? 2 :
      props->chroma_resampler == "sinc" ? 3 : 
      props->chroma_resampler == "lanczos" ? 4 :
      1),
    "dither", (
      props->dither == "none" ? 0 :
      props->dither == "verterr" ? 1 :
      props->dither == "floyd-steinberg" ? 2 :
      props->dither == "sierra-lite" ? 3 : 
      props->dither == "bayer" ? 4 :
      3),
    "method", (
      props->method == "nearest-neighbour" ? 0 :
      props->method == "bilinear" ? 1 :
      props->method == "4-tap" ? 2 :
      props->method == "lanczos" ? 3 : 
      props->method == "bilinear2" ? 4 :
      props->method == "sinc" ? 5 :
      props->method == "hermite" ? 6 :
      props->method == "spline" ? 7 :
      props->method == "catrom" ? 8 : 
      props->method == "mitchell" ? 9 :
      1),
  NULL);
};

#endif
