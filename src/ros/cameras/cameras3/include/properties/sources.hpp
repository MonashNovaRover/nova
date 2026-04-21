#ifndef SOURCES_HEADER
#define SOURCES_HEADER

#include <string>
#include <gst/gst.h>

template<typename properties> void set_v4lsource(GstElement* source, const properties props) {
  g_object_set(source,
    "device", props->device.c_str(),
    "io-mode", (
      props->io_mode == "rw" ? 1 :
      props->io_mode == "mmap" ? 2 :
      props->io_mode == "userptr" ? 3 :
      props->io_mode == "dmabuf" ? 4 :
      props->io_mode == "dmabuf-import" ? 5 :
      0),
  NULL);
}

#endif
