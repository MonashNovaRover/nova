#include "cameras/cameras.hpp"
#include <optional>
#include <string>

#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"

#include <camera_msgs/msg/camera.hpp>


//figure out how to return gstelement address or null
GstElement* v4l2webrtc_pipeline(rclcpp::Node* log_node, v4l2webrtcPipelineProperties* props)
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
      RCLCPP_ERROR(log_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }
  RCLCPP_INFO(log_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate);
  g_object_set(source, "device", props->node.c_str(), NULL);
  GstCaps *caps = gst_caps_new_simple(
      props->mime.c_str(),
      "width", G_TYPE_INT, props->width,
      "height", G_TYPE_INT, props->height,
      "framerate", GST_TYPE_FRACTION, props->framerate, 1, NULL);
  g_object_set(filter, "caps", caps, NULL);
  gst_caps_unref(caps);
  GstStructure *meta = gst_structure_new("meta", "serial", G_TYPE_STRING, props->serial.c_str(), NULL); 
  g_object_set(webrtc,
      "do-fec", props->do_fec,
      "do-retransmission", props->do_retransmission,
      "congestion-control", (
      props->congestion_control == "disabled" ? 0 :
      props->congestion_control == "homegrown" ? 1 :
      props->congestion_control == "gcc" ? 2 : -1),
      "meta", meta, 
      NULL);
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
      RCLCPP_ERROR(log_node->get_logger(), "Could not link elements of pipeline for %s", props->serial.c_str());
      return nullptr;
  }

  return gst_pipeline;
}

v4l2webrtcPipelineProperties* get_v4l2webrtc_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera camera)
{
  return nullptr;
  // v4l2webrtcPipelineProperties* props; 
  // camera_streamer_service::Params params = param_listener->get_params();
  // std::map<std::string, rclcpp::Parameter> serial_params;

  // // set defaults
  // props->width = params.defaults.camera_properties.width;
  // props->height = params.defaults.camera_properties.height;
  // props->framerate = params.defaults.camera_properties.framerate;
  // props->mime = params.defaults.camera_properties.mime;
  // props->congestion_control = params.defaults.pipeline_properties.congestion_control;
  // props->do_fec = params.defaults.pipeline_properties.do_fec;
  // props->do_retransmission = params.defaults.pipeline_properties.do_retransmission;
  // props->show_clock = params.defaults.pipeline_properties.show_clock;

  // // override any defaults with params
  // log_node->get_parameters("cameras." + serial, serial_params);
  // RCLCPP_INFO(log_node->get_logger(), "get props for %s", serial.c_str());
  // if (!serial_params.empty()) {
  //   std::map<std::string, rclcpp::Parameter> camera_params;
  //   log_node->get_parameters("cameras." + serial + ".camera_properties", camera_params);
  //   if (!camera_params.empty()) {
  //     props->width = camera_params.find("width") != camera_params.end() ? camera_params["width"].as_int() : props->width;
  //     props->height = camera_params.find("height") != camera_params.end() ? camera_params["height"].as_int() : props->height;
  //     props->framerate = camera_params.find("framerate") != camera_params.end() ? camera_params["framerate"].as_int() : props->framerate;
  //     props->mime = camera_params.find("mime") != camera_params.end() ? camera_params["mime"].as_string() : props->mime;
  //   }
  //   std::map<std::string, rclcpp::Parameter> pipeline_params;
  //   log_node->get_parameters("cameras." + serial + ".pipeline_properties", pipeline_params);
  //   if (!camera_params.empty()) {
  //     props->congestion_control = pipeline_params.find("congestion_control") != pipeline_params.end() ? pipeline_params["congestion_control"].as_string() : props->congestion_control;
  //     props->do_fec = pipeline_params.find("do_fec") != pipeline_params.end() ? pipeline_params["do_fec"].as_bool() : props->do_fec;
  //     props->do_retransmission = pipeline_params.find("do_retransmission") != pipeline_params.end() ? pipeline_params["do_retransmission"].as_bool() : props->do_retransmission;
  //     props->show_clock = pipeline_params.find("show_clock") != pipeline_params.end() ? pipeline_params["show_clock"].as_bool() : props->show_clock;
  //   }
    // RCLCPP_INFO(log_node->get_logger(), "params, %d, %d, %d, %s, %s, %d, %d, %d", 
    //   props->width, props->height, 
    //   props->framerate, props->mime.c_str(),
    //   props->congestion_control.c_str(), props->do_fec,
    //   props->do_retransmission, props->show_clock
    // );
  // }
}