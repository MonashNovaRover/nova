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
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");
  GstElement* payload = (props->payload_quirk) ? gst_element_factory_make("rtph264pay", "payloader") : nullptr;
  GstElement* depayload = (props->payload_quirk) ? gst_element_factory_make("rtph264depay", "depayloader") : nullptr;
  GstElement* parse = (props->payload_quirk) ? gst_element_factory_make("h264parse", "parser") : nullptr;

  if (!gst_pipeline || !source || !srcfilter || !webrtc || (props->payload_quirk && !payload) || (props->payload_quirk && !depayload) || (props->payload_quirk && !parse)) {
      RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create pipeline for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
      return nullptr;
  }

  // 2. Set element properties
  set_v4lsource(source, props);
  set_srcfilter(srcfilter, props);
  if (props->payload_quirk) {
    set_h264payload(payload);
    set_h264parse(parse, -1);
  }
  set_webrtcsink(webrtc, props);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, srcfilter, webrtc, NULL);
  if (props->payload_quirk) gst_bin_add_many(GST_BIN(gst_pipeline), payload, depayload, parse, NULL);

  // 4. Link elements
  
  GstElement* next_element = source;
 
  if (link_elements(streamer_node, next_element, srcfilter, props->serial)) next_element = srcfilter;
  else {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sWrong resolution for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }
  if (link_elements(streamer_node, next_element, payload, props->serial)) next_element = payload;
  if (link_elements(streamer_node, next_element, depayload, props->serial)) next_element = depayload;
  if (link_elements(streamer_node, next_element, parse, props->serial)) next_element = parse;
  link_elements(streamer_node, next_element, webrtc, props->serial);

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
  props->device = camera->node;

  props->io_mode = 4; // dmabuf

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

