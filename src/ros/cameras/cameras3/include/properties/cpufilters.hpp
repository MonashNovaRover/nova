#ifndef CPUFILTERS_HEADER
#define CPUFILTERS_HEADER

#include <algorithm>
#include <gst/gst.h>

int crop43(const int width, const int height, const float zoom);

void set_queue(GstElement* queue);

template<typename properties> void set_cpu_crop43(GstElement* element, const properties& props) {
  const int crop_width = props->crop43 ? crop43(props->width, props->height, props->zoom) : 0;
  const int zoom_width = (props->width - (int)((float)props->width/props->zoom)) / 2;
  const int zoom_height = (props->height - (int)((float)props->height/props->zoom)) / 2;
  const int zoom_longitude_bias = std::clamp(props->zoom_longitude, -1.0f, 1.0f) * zoom_width;
  const int zoom_latitude_bias = std::clamp(props->zoom_latitude, -1.0f, 1.0f) * zoom_height;
  g_object_set(element,
    "bottom", zoom_height - zoom_latitude_bias,
    "left", crop_width + zoom_width - zoom_longitude_bias,
    "right", crop_width + zoom_width + zoom_longitude_bias,
    "top", zoom_height + zoom_latitude_bias,
  NULL);
};

template<typename properties> void set_cpu_crop43(GstElement* element, const properties& props, const int crop_width) {
  g_object_set(element,
    "left", crop_width,
    "right", crop_width,
  NULL);
};

template<typename properties> void set_convertscale(GstElement* element, const properties& props) {
  g_object_set(element,
    "chroma-resampler", (
      props->chroma_resampler == "nearest" ? 0 :
      props->chroma_resampler == "linear" ? 1 :
      props->chroma_resampler == "cubic" ? 2 :
      props->chroma_resampler == "sinc" ? 3 : 
      props->chroma_resampler == "lanczos" ? 4 :
      2),
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
      9),
  NULL);
};

#endif
