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

GstElement* h264passthrough_pipeline(rclcpp::Node* streamer_node, h264passthroughPipelineProperties* props)
{
  // 1. Create the elements
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* parse = gst_element_factory_make("h264parse", "parser");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");
  GstElement* payload = (props->payload_quirk) ? gst_element_factory_make("rtph264pay", "payloader") : nullptr;
  GstElement* depayload = (props->payload_quirk) ? gst_element_factory_make("rtph264depay", "depayloader") : nullptr;

  if (!gst_pipeline || !source || !srcfilter || !parse || !webrtc || (props->payload_quirk && !payload) || (props->payload_quirk && !depayload)) {
      RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create pipeline for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
      return nullptr;
  }

  // 2. Set element properties
  set_v4lsource(source, props->device, props->io_mode);
  set_srcfilter(srcfilter, props->mime, props->width, props->height, props->framerate, props->framerate_denominator, props->downrate, props->brightness, props->contrast);
  if (props->payload_quirk) set_h264payload(payload);
  set_h264parse(parse);
  set_webrtcsink(webrtc, props->serial, props->video_caps, props->do_fec, props->do_retransmission, props->congestion_control, props->bitrate);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, srcfilter, parse, webrtc, NULL);
  if (props->payload_quirk) gst_bin_add_many(GST_BIN(gst_pipeline), payload, depayload, NULL);

  // 4. Link elements
  if (!link_elements(streamer_node, source, srcfilter, props->serial)) return nullptr;

  if (props->payload_quirk) {
    if (!link_elements(streamer_node, srcfilter, payload, props->serial)) return nullptr;
    if (!link_elements(streamer_node, payload, depayload, props->serial)) return nullptr;
    if (!link_elements(streamer_node, depayload, parse, props->serial)) return nullptr;
  } else {
    if (!link_elements(streamer_node, srcfilter, parse, props->serial)) return nullptr;
  }
  if (!link_elements(streamer_node, parse, webrtc, props->serial)) return nullptr;

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for h264passthrough pipeline or sets defaults
*/

h264passthroughPipelineProperties* get_h264passthrough_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  // 0. Initialize constants
  h264passthroughPipelineProperties* props = new h264passthroughPipelineProperties;
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = camera->node;

  props->io_mode = 4; // dmabuf

  props->verify_resolution = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "verify_resolution", false);

  // rate
  props->downrate = 1; // Do not change framerate

  // filter
  props->mime = "video/x-h264";

  props->brightness = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "brightness", 0);
  props->contrast = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "contrast", 0);
  props->framerate = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "framerate", 30);
  props->framerate_denominator = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "framerate_denominator", 1);
  props->height = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "height", 720);
  props->width = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "width", 1280);

  // payloader
  props->payload_quirk = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "payload_quirk", false);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "congestion_control", default_string);
  props->video_caps = "video/x-h264";

  props->bitrate = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "bitrate", 4096);

  props->do_fec = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "do_retransmission", false);

  // 2. Finalize props
  if (props->verify_resolution) {
    if (verify_v4lresolution(props->device, &props->mime, &props->width, &props->height, &props->framerate, &props->framerate_denominator)) {
        RCLCPP_INFO(streamer_node->get_logger(), "%sInitialized pipeline: %s%s%s for %s%s%s with profile: %s%s %dx%d@%dfps%s", C_QUIET, C_INPUT, camera->pipeline_type.c_str(), C_QUIET, C_TITLE, props->serial.c_str(), C_QUIET, C_MODE, camera->profile.c_str(), props->width, props->height, props->framerate/props->framerate_denominator, C_RESET);
    } else {
        RCLCPP_ERROR(streamer_node->get_logger(), "%sWrong resolution!%s Fallback pipeline: %s%s%s for %s%s%s with profile: %s%s %dx%d@%dfps%s", C_FAIL, C_QUIET, C_INPUT, camera->pipeline_type.c_str(), C_QUIET, C_TITLE, props->serial.c_str(), C_QUIET, C_MODE, camera->profile.c_str(), props->width, props->height, props->framerate/props->framerate_denominator, C_RESET);
    }
  } else {
      RCLCPP_INFO(streamer_node->get_logger(), "%sInitialized pipeline: %s%s%s for %s%s%s with profile: %s%s %dx%d@%dfps%s", C_QUIET, C_INPUT, camera->pipeline_type.c_str(), C_QUIET, C_TITLE, props->serial.c_str(), C_QUIET, C_MODE, camera->profile.c_str(), props->width, props->height, props->framerate/props->framerate_denominator, C_RESET);
  }

  return props;
}

