#ifndef SINKS_HEADER
#define SINKS_HEADER

#include <string>
#include <gst/gst.h>

template<typename properties> void set_webrtcsink(GstElement* element, const properties& props) {
  GstStructure *meta = gst_structure_new("meta", "serial", G_TYPE_STRING, props->serial.c_str(), NULL); 
  GstCaps *webrtc_caps = gst_caps_from_string(props->video_caps.c_str());
  g_object_set(element,
    "do-fec", props->do_fec,
    "do-retransmission", props->do_retransmission,
    "congestion-control", (
      props->congestion_control == "disabled" ? 0 :
      props->congestion_control == "homegrown" ? 1 :
      props->congestion_control == "gcc" ? 2 :
      2),
    "max-bitrate", props->bitrate*1125,
    "meta", meta,
    "video-caps", webrtc_caps,
    NULL);
  gst_caps_unref(webrtc_caps);
  gst_structure_free(meta);
}

template<typename properties> void set_rostopicsink(GstElement* element, const properties& props) {
  g_object_set(element, "ros-topic", props->ros_topic.c_str(), NULL);
}

#endif
