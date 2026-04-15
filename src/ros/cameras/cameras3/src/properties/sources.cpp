#include <string>
#include <gst/gst.h>
#include "properties/sources.hpp"

void set_v4lsource(GstElement* source, const std::string device, const std::string io_mode) {
    g_object_set(source,
      "device", device.c_str(),
      "io-mode", (
          io_mode == "rw" ? 1 :
          io_mode == "mmap" ? 2 :
          io_mode == "userptr" ? 3 :
          io_mode == "dmabuf" ? 4 :
          io_mode == "dmabuf-import" ? 5 :
          0),
      NULL);
}

