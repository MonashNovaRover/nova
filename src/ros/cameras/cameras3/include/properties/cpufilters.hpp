#ifndef CPUFILTERS_HEADER
#define CPUFILTERS_HEADER

#include <algorithm>
#include <gst/gst.h>

int crop43(const int width, const int height, const float zoom);

void set_queue(GstElement* queue);

template<typename properties>
void set_cpu_crop43(GstElement* element, const properties& props) {
  const double width = props->width;
  const double height = props->height;

  // Base crop: 16:9 -> 4:3.
  const double base_width = props->crop43
    ? height * 4.0 / 3.0
    : width;

  // Apply zoom.
  const double crop_width = base_width / props->zoom;
  const double crop_height = height / props->zoom;

  // Maximum movement available after zooming.
  const double max_x = base_width - crop_width;
  const double max_y = height - crop_height;

  const double x = std::clamp(props->zoom_longitude, -1.0, 1.0);
  const double y = std::clamp(props->zoom_latitude, -1.0, 1.0);

  // Move the crop window from left/top to right/bottom.
  const double offset_x = max_x * (x + 1.0) / 2.0;
  const double offset_y = max_y * (y + 1.0) / 2.0;

  // Centre the 4:3 region inside the original 16:9 frame.
  const double base_left = (width - base_width) / 2.0;

  const double left = base_left + offset_x;
  const double right = width - base_left - crop_width - offset_x;

  const double top = offset_y;
  const double bottom = height - crop_height - offset_y;

  g_object_set(element,
    "left",   (int)left,
    "right",  (int)right,
    "top",    (int)top,
    "bottom", (int)bottom,
    NULL);
}

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
