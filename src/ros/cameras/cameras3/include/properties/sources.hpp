#ifndef SOURCES_HEADER
#define SOURCES_HEADER

#include <string>
#include <gst/gst.h>

inline void add_control(GstStructure* str, const char* name, const int value) {
  if (value != -1)
    gst_structure_set(str, name, G_TYPE_INT, value, NULL);
}

template<typename properties> void set_v4lsource(GstElement* element, const properties& props) {
  GstStructure *str = gst_structure_new_empty("controls");

  add_control(str, "brightness", props->brightness);
  add_control(str, "contrast", props->contrast);
  add_control(str, "saturation", props->saturation);
  add_control(str, "gain", props->gain);
  add_control(str, "gamma", props->gamma);
  add_control(str, "sharpness", props->sharpness);
  add_control(str, "backlight_compensation", props->backlight_compensation);

  if (props->exposure != -1) {
    add_control(str, "auto_exposure", 1); // Manual exposure
    add_control(str, "exposure_time_absolute", props->exposure);
  } else {
    add_control(str, "auto_exposure", 3); // Auto exposure
  }

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

template<typename properties> void set_rtspsource(GstElement* element, const properties& props) {
  g_object_set(element,
    "location", props->url.c_str(),
    "buffer-mode", 4, // synced
    "do-retransmission", props->do_retransmission, // No retransmission, keep latency low
    "drop-on-latency", true, // Keeps latency below set ms
    "latency", props->latency,
    "ntp-sync", true, // Sync to computer
    "ntp-time-source", 1, // unix, works best on linux devices
    "onvif-mode", true, // enable onvif mode
    "protocols", (
      props->rtsp_protocol == "udp" ? 1 :
      props->rtsp_protocol == "udp_mcast" ? 2 :
      props->rtsp_protocol == "tcp" ? 4:
      4), // default tcp, which seems to be the most compatible right now
    "tcp-timestamp", (props->rtsp_protocol == "tcp"),
  NULL);
}

#endif
