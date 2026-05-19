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

#include "properties/h264.hpp"
#include "cameras/colors.hpp"

/*
 * V4l camera (h264) to webrtc pipeline (direct)
 * Enforces alignment from h264 v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-h264
 */

GstElement* h264passthrough_pipeline(rclcpp::Node* streamer_node, const std::unique_ptr<h264passthroughPipelineProperties>& props)
{
  // 1. Create the elements
  std::string section = "source";
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source_v4l = gst_element_factory_make("v4l2src", "source_v4l");
  GstElement* source_valve = gst_element_factory_make("valve", "source_valve");
  GstElement* source_filter = gst_element_factory_make("capsfilter", "source_filter");

  if (
    !gst_pipeline ||
    !source_v4l ||
    !source_valve ||
    !source_filter
  ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  section = "h264";
  GstElement* h264_payload = (props->payload_quirk) ? gst_element_factory_make("rtph264pay", "h264_payload") : nullptr;
  GstElement* h264_depayload = (props->payload_quirk) ? gst_element_factory_make("rtph264depay", "h264_depayload") : nullptr;
  GstElement* h264_parse = gst_element_factory_make("h264parse", "h264_parse");

  if ((props->payload_quirk && (
    !h264_payload ||
    !h264_depayload
  )) || !h264_parse) {
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
    source_v4l,
    source_valve,
    source_filter,
    h264_parse,
    webrtc_sink,
  NULL);
  if (props->payload_quirk) gst_bin_add_many(GST_BIN(gst_pipeline), h264_payload, h264_depayload, NULL);

  // 3. Set element properties
  set_v4lsource(source_v4l, props);
  set_srcfilter(source_filter, props);

  if (props->payload_quirk) {
    set_h264payload(h264_payload);
  }
  set_h264parse(h264_parse, -1);

  set_webrtcsink(webrtc_sink, props);

  // 4. Link elements
  
  GstElement* next_element = source_v4l;
 
  link_elements(streamer_node, next_element, source_valve, props->serial);
  if (!link_elements(streamer_node, next_element, source_filter, props->serial)) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sWrong resolution for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  link_elements(streamer_node, next_element, h264_payload, props->serial);
  link_elements(streamer_node, next_element, h264_depayload, props->serial);
  link_elements(streamer_node, next_element, h264_parse, props->serial);

  link_elements(streamer_node, next_element, webrtc_sink, props->serial);

  next_element = nullptr;

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for h264passthrough pipeline or sets defaults
*/

std::unique_ptr<h264passthroughPipelineProperties> get_h264passthrough_pipeline_properties(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera)
{
  // 0. Initialize constants
  std::unique_ptr<h264passthroughPipelineProperties> props = std::make_unique<h264passthroughPipelineProperties>();
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = set_property(streamer_node, camera, "device", camera->node);

  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera, "io_mode", default_string);

  // scale
  props->downscale = 1; // Do not change scale

  // rate
  props->downrate = 1; // Do not change framerate

  // filter
  props->mime = "video/x-h264";

  props->brightness = set_property(streamer_node, camera, "brightness", 0);
  props->contrast = set_property(streamer_node, camera, "contrast", 0);
  props->framerate = set_property(streamer_node, camera, "framerate", 30);
  props->framerate_denominator = set_property(streamer_node, camera, "framerate_denominator", 1);
  props->height = set_property(streamer_node, camera, "height", 720);
  props->width = set_property(streamer_node, camera, "width", 1280);

  // payloader
  props->payload_quirk = set_property(streamer_node, camera, "payload_quirk", false);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera, "congestion_control", default_string);
  props->video_caps = "video/x-h264";

  props->bitrate = set_property(streamer_node, camera, "bitrate", 4096);

  props->do_fec = set_property(streamer_node, camera, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera, "do_retransmission", false);

  // 2. Finalize props
  display_resolution(streamer_node, props, camera, 0);

  return props;
}

void set_h264passthrough_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<h264passthroughPipelineProperties>& props) {

  // 1. Initialize constants
  GstElement *source_valve = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_valve");
  GstElement* source_filter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_filter");

  // 2. Set properties for elements
  g_object_set(source_valve, "drop", true, NULL);

  if (source_filter) {
    set_srcfilter(source_filter, props);
    gst_object_unref(source_filter);
  }

  g_object_set(source_valve, "drop", false, NULL);

  // 4. Unreference every element
  gst_object_unref(source_valve);
}
