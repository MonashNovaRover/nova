#ifndef SOURCES_HEADER
#define SOURCES_HEADER

#include <string>
#include <gst/gst.h>

template<typename properties> void set_v4lsource(GstElement* element, const properties& props) {
  GstStructure *str = gst_structure_new(
    "controls",
    "brightness", G_TYPE_INT, props->brightness,
    "contrast", G_TYPE_INT, props->contrast,
    "saturation", G_TYPE_INT, props->saturation,
    "gain", G_TYPE_INT, props->gain,
    "sharpness", G_TYPE_INT, props->sharpness,
  NULL);
  g_object_set(element,
    "device", props->device.c_str(),
    "io-mode", (
      props->io_mode == "rw" ? 1 :
      props->io_mode == "mmap" ? 2 :
      props->io_mode == "userptr" ? 3 :
      props->io_mode == "dmabuf" ? 4 :
      props->io_mode == "dmabuf-import" ? 5 :
      0),
      "extra-controls", str,
  NULL);
  gst_structure_free(str);
}

#endif
