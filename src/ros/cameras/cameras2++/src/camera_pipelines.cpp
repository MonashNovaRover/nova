#include "cameras/cameras.hpp"
#include <optional>
#include <string>

#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"

#include <camera_msgs/msg/camera.hpp>


GstElement* v4l2webrtc_pipeline(rclcpp::Node* streamer_node, v4l2webrtcPipelineProperties* props)
{
  // A simple v4l camera to webrtc pipeline
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* filter = gst_element_factory_make("capsfilter", "filter");
  GstElement* decode = gst_element_factory_make("decodebin", "decoder");
  GstElement* convert = gst_element_factory_make("videoconvert", "converter");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");
  GstElement* clock = props->show_clock ? gst_element_factory_make("clockoverlay", "clock") : nullptr;

  if (!gst_pipeline || !source || !filter || !decode || !convert || !webrtc
      || (props->show_clock && !clock) 
      ) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }
  RCLCPP_INFO(streamer_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate);
  g_object_set(source, "device", props->node.c_str(), NULL);
  GstCaps *caps = gst_caps_new_simple(
      props->mime.c_str(),
      "width", G_TYPE_INT, props->width,
      "height", G_TYPE_INT, props->height,
      "framerate", GST_TYPE_FRACTION, props->framerate, 1, NULL);
  g_object_set(filter, "caps", caps, NULL);
  gst_caps_unref(caps);

  GstStructure *meta = gst_structure_new("meta", "serial", G_TYPE_STRING, props->serial.c_str(), NULL); 
  GstCaps *webrtc_caps = gst_caps_from_string(props->video_caps.c_str());
  g_object_set(webrtc,
      "do-fec", props->do_fec,
      "do-retransmission", props->do_retransmission,
      "congestion-control", (
        props->congestion_control == "disabled" ? 0 :
        props->congestion_control == "homegrown" ? 1 :
        props->congestion_control == "gcc" ? 2 : -1),
      "meta", meta, 
      "video-caps", webrtc_caps,
      NULL);
  gst_caps_unref(webrtc_caps);
  gst_structure_free(meta);

  gst_bin_add_many(GST_BIN(gst_pipeline), source, filter, decode, convert, webrtc, NULL);

  g_signal_connect(decode, "pad-added", G_CALLBACK(+[](GstElement* /*decode*/, GstPad* new_pad, gpointer user_data) {
      GstElement* convert = static_cast<GstElement*>(user_data);
      GstPad* sink_pad = gst_element_get_static_pad(convert, "sink");
      if (sink_pad && !gst_pad_is_linked(sink_pad)) {
          gst_pad_link(new_pad, sink_pad);
      }
      if (sink_pad) gst_object_unref(sink_pad);
  }), convert);

  bool ret = true;

  ret = gst_element_link(source, filter) ? ret : false;
  ret = gst_element_link(filter, decode) ? ret : false;

  if (props->show_clock) {
      gst_bin_add(GST_BIN(gst_pipeline), clock);
      ret = gst_element_link(convert, clock) ? ret : false;
      ret = gst_element_link(clock, webrtc) ? ret : false;
  } else {
      ret = gst_element_link(convert, webrtc) ? ret : false;
  }

  if (!ret) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not link elements of pipeline for %s", props->serial.c_str());
      return nullptr;
  }

  return gst_pipeline;
}

v4l2webrtcPipelineProperties* get_v4l2webrtc_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  v4l2webrtcPipelineProperties* props = new v4l2webrtcPipelineProperties; 

  std::map<std::string, rclcpp::Parameter> serial_params;

  RCLCPP_INFO(streamer_node->get_logger(), "Getting props for %s", camera->serial.c_str());
  props->serial = camera->serial;
  props->node = camera->node;

  // override any defaults with params
  std::string camera_prefix = std::string(PIPELINE_PREFIX) + "." + camera->serial;
  streamer_node->get_parameter_or((camera_prefix + ".width").c_str(), props->width, 640); 
  streamer_node->get_parameter_or((camera_prefix + ".height").c_str(), props->height, 480); 
  streamer_node->get_parameter_or((camera_prefix + ".framerate").c_str(), props->framerate, 10); 
  streamer_node->get_parameter_or<std::string>((camera_prefix + ".mime").c_str(), props->mime, "image/jpeg"); 
  streamer_node->get_parameter_or<std::string>((camera_prefix + ".congestion_control").c_str(), props->congestion_control, "gcc"); 
  streamer_node->get_parameter_or((camera_prefix + ".do_fec").c_str(), props->do_fec, false); 
  streamer_node->get_parameter_or((camera_prefix + ".do_retransmission").c_str(), props->do_retransmission, false); 
  streamer_node->get_parameter_or((camera_prefix + ".show_clock").c_str(), props->show_clock, false); 
  streamer_node->get_parameter_or<std::string>((camera_prefix + ".video_caps").c_str(), props->video_caps, "video/x-vp8"); 

  // RCLCPP_INFO(streamer_node->get_logger(), "params, %d, %d, %d, %s, %s, %d, %d, %d", 
  //   props->width, props->height, 
  //   props->framerate, props->mime.c_str(),
  //   props->congestion_control.c_str(), props->do_fec,
  //   props->do_retransmission, props->show_clock
  // );

  return props;
}