#include <string>

#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"
#include <camera_msgs/msg/camera.hpp>

#include "pipelines/properties.hpp"
#include "pipelines/pipelines.hpp"
#include "properties/common.hpp"

#include "properties/sources.hpp"
#include "properties/sinks.hpp"

#include "properties/capsfilters.hpp"
#include "properties/cpufilters.hpp"

#include "properties/h26X.hpp"
#include "cameras/colors.hpp"

/*
 * UVC theta camera (h264) to webrtc pipeline (direct)
 * Enforces alignment from uvc theta camera and feeds directly to webrtc 
 * gst-launch-1.0 thetauvcsrc mode=2K ! valve ! queue ! h264parse ! webrtc
sink do-fec=true do-retransmission=true congestion-control=gcc meta='meta, serial=(string)mast_forward'
*/

GstElement* theta_pipeline(rclcpp::Node* streamer_node, const std::unique_ptr<thetaPipelineProperties>& props)
{
  // 1. Create the elements
  std::string section = "source";
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source_theta = gst_element_factory_make("thetauvcsrc", "source_theta");
  GstElement* source_valve = gst_element_factory_make("valve", "source_valve");
  GstElement* source_queue = gst_element_factory_make("queue", "source_queue");

  if (
    !gst_pipeline ||
    !source_theta ||
    !source_valve ||
    !source_queue
  ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  section = "h264";
  GstElement* h264_parse = gst_element_factory_make("h264parse", "h264_parse");

  if (!h264_parse) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  section = "sink"; 
  GstElement* webrtc_sink = gst_element_factory_make("webrtcsink", "webrtc_sink");

  if (
    !webrtc_sink
  ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  // 2. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline),
    source_theta,
    source_valve,
    source_queue,
    h264_parse,
    webrtc_sink,
  NULL);

  // 3. Set element properties
  set_thetasource(source_theta);
  set_queue(source_queue);

  set_h264parse(h264_parse, -1);

  set_webrtcsink(webrtc_sink, props);

  // 4. Link elements
  
  GstElement* next_element = source_theta;
 
  link_elements(streamer_node, next_element, source_valve, props->serial);
  link_elements(streamer_node, next_element, source_queue, props->serial);

  link_elements(streamer_node, next_element, h264_parse, props->serial);

  link_elements(streamer_node, next_element, webrtc_sink, props->serial);

  next_element = nullptr;

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for theta pipeline or sets defaults
*/

std::unique_ptr<thetaPipelineProperties> get_theta_pipeline_properties(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera)
{
  // 0. Initialize constants
  std::unique_ptr<thetaPipelineProperties> props = std::make_unique<thetaPipelineProperties>();
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // 1. Define default properties
  std::string default_string;

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera, "congestion_control", default_string);
  props->video_caps = "video/x-h264";

  props->bitrate = set_property(streamer_node, camera, "bitrate", 4096);

  props->do_fec = set_property(streamer_node, camera, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera, "do_retransmission", false);

  // 2. Finalize props
  //display_resolution(streamer_node, props, camera, 0);

  return props;
}

void set_theta_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<thetaPipelineProperties>& props) {

  // Literally does nothing

  // 1. Initialize constants
  GstElement *source_valve = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_valve");

  // 2. Set properties for elements
  g_object_set(source_valve, "drop", true, NULL);

  g_object_set(source_valve, "drop", false, NULL);

  // 4. Unreference every element
  gst_object_unref(source_valve);
}
