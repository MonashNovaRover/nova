#ifndef CPUFILTERS_HEADER
#define CPUFILTERS_HEADER

#include <algorithm>
#include <gst/gst.h>

int crop43(const int width, const int height, const float zoom);

void set_queue(GstElement* queue);

template<typename properties> void set_cpu_crop43(GstElement* element, const properties& props) {
  // Find the max dimensions if unzoomed
  const double max_width = props->crop43 ? props->width - crop43(props->width, props->height, props->zoom) : props->width;
  const double max_height = props->height;

  // Find the size of the zoom window
  const double zoom_width = max_width/props->zoom;
  const double zoom_height = max_height/props->zoom;

  // Find the centre of the zoom window
  const double zoom_longitude_bias = std::clamp(props->zoom_longitude, -1.0, 1.0);
  const double zoom_latitude_bias = std::clamp(props->zoom_latitude, -1.0, 1.0);

  const double center_x = max_width / 2 + zoom_longitude_bias * (max_width - zoom_width) / 2;

  const double center_y = max_height / 2 + zoom_latitude_bias * (max_height - zoom_height) / 2;

  g_object_set(element,
    "left", (int) (center_x - zoom_width/2),
    "right", (int) (max_width - center_x - zoom_width/2),
    "top", (int) (center_y - zoom_height/2),
    "bottom", (int) (max_height - center_y - zoom_height/2),
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
