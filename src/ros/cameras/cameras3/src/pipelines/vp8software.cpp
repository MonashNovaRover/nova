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

#include "properties/software_encoders.hpp"
#include "cameras/colors.hpp"

/*
 * V4l camera (any) decoded then encoded into vp8enc
 * Enforces alignment from vp8 v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-vp8
 */

GstElement* vp8software_pipeline(rclcpp::Node* streamer_node, vp8softwarePipelineProperties* props)
{
  // 0. Initialize constants

  // Verify resolution
  const std::string pipeline_type = "vp8software";
  if (props->verify_resolution) {
    if (verify_v4lresolution(props->device, &props->mime, &props->width, &props->height, &props->framerate, &props->framerate_denominator)) {
        RCLCPP_INFO(streamer_node->get_logger(), "%sStarting %s%s%s pipeline for %s%s%s with %s%dx%d@%dfps%s", C_QUIET, C_INPUT, pipeline_type.c_str(), C_QUIET, C_TITLE, props->serial.c_str(), C_QUIET, C_MODE, props->width, props->height, props->framerate/props->framerate_denominator, C_RESET);
    } else {
        RCLCPP_ERROR(streamer_node->get_logger(), "%sWrong resolution!%s Fallback %s%s%s pipeline for %s%s%s with %s%dx%d@%dfps%s", C_FAIL, C_QUIET, C_INPUT, pipeline_type.c_str(), C_QUIET, C_TITLE, props->serial.c_str(), C_QUIET, C_MODE, props->width, props->height, props->framerate/props->framerate_denominator, C_RESET);
    }
  } else {
      RCLCPP_INFO(streamer_node->get_logger(), "%sStarting %s%s%s pipeline for %s%s%s with %s%dx%d@%dfps%s", C_QUIET, C_INPUT, pipeline_type.c_str(), C_QUIET, C_TITLE, props->serial.c_str(), C_QUIET, C_MODE, props->width, props->height, props->framerate/props->framerate_denominator, C_RESET);
  }
  // Disable crop43 if it is already 4:3
  const int crop_width = crop43(props->width, props->height);
  if (crop_width == 0) {
      props->crop43 = false;
  }

  // 1. Create the elements
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* rate = (props->downrate > 1) ? gst_element_factory_make("videorate", "rate") : nullptr;
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* decode = (props->mime == "image/jpeg") ? gst_element_factory_make(props->decoder.c_str(), "decoder") : nullptr;
  GstElement* convert = gst_element_factory_make("videoconvertscale", "converter");
  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* encode = gst_element_factory_make("vp8enc", "encoder");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");
  GstElement* clock = (props->show_clock) ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* cropper = (props->crop43) ? gst_element_factory_make("videocrop", "video-cropper") : nullptr;

  if (!gst_pipeline || !source || (props->downrate > 1 && !rate) || !srcfilter || (props->mime == "image/jpeg" && !decode) || !convert || !scalefilter  || (props->show_clock && !clock) || (props->crop43 && !cropper) || !encode || !webrtc) {
      RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create pipeline for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
      return nullptr;
  }
  
  // 2. Set element properties
  set_v4lsource(source, props->device, props->io_mode);
  set_srcfilter(srcfilter, props->mime, props->width, props->height, props->framerate, props->framerate_denominator, props->downrate, props->brightness, props->contrast);
  set_convertscale(convert, props->chroma_resampler, props->dither, props->method);
  set_scalefilter(scalefilter, props->format, props->width, props->height, props->framerate, props->framerate_denominator, props->downscale, props->downrate, props->brightness, props->contrast);
  set_crop43(cropper, props->crop43, crop_width, props->downscale);
  set_vp8enc(encode, props->cpu_used, props->threads, props->bitrate, props->gop, props->framerate, props->framerate_denominator, props->downrate);
;
  set_webrtcsink(webrtc, props->serial, props->video_caps, props->do_fec, props->do_retransmission, props->congestion_control, props->bitrate);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, srcfilter, convert, scalefilter, encode, webrtc, NULL);
  if (props->crop43) gst_bin_add(GST_BIN(gst_pipeline), cropper);
  if (props->show_clock) gst_bin_add(GST_BIN(gst_pipeline), clock);
  if (props->mime == "image/jpeg") gst_bin_add(GST_BIN(gst_pipeline), decode);
  if (props->downrate > 1) gst_bin_add(GST_BIN(gst_pipeline), rate);

  // 4. Link elements
  
  // Change fps
  if (props->downrate > 1) {
    if (!link_elements(streamer_node, source, rate, props->serial)) return nullptr;    
    if (!link_elements(streamer_node, rate, srcfilter, props->serial)) return nullptr;

  } else {
    if (!link_elements(streamer_node, source, srcfilter, props->serial)) return nullptr;
  }

  // Convert to raw
  if (props->mime == "image/jpeg") {
      if (!link_elements(streamer_node, srcfilter, decode, props->serial)) return nullptr;
      if (!link_elements(streamer_node, decode, convert, props->serial)) return nullptr;
  } else {
      if (!link_elements(streamer_node, srcfilter, convert, props->serial)) return nullptr;
  }
  
  if (!link_elements(streamer_node, convert, scalefilter, props->serial)) return nullptr;

  // Enable crop and/or clock
  if (props->crop43 && props->show_clock) {
      if (!link_elements(streamer_node, scalefilter, cropper, props->serial)) return nullptr;
      if (!link_elements(streamer_node, cropper, clock, props->serial)) return nullptr;
      if (!link_elements(streamer_node, clock, encode, props->serial)) return nullptr;
  } else if (props->crop43) {
      if (!link_elements(streamer_node, scalefilter, cropper, props->serial)) return nullptr;
      if (!link_elements(streamer_node, cropper, encode, props->serial)) return nullptr;
  } else if (props->show_clock) {
      if (!link_elements(streamer_node, scalefilter, clock, props->serial)) return nullptr;
      if (!link_elements(streamer_node, clock, encode, props->serial)) return nullptr;
  } else {
      if (!link_elements(streamer_node, scalefilter, encode, props->serial)) return nullptr;
  }

  if (!link_elements(streamer_node, encode, webrtc, props->serial)) return nullptr;

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for vp8software pipeline or sets defaults
*/

vp8softwarePipelineProperties* get_vp8software_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  // 0. Initialize constants
  vp8softwarePipelineProperties* props = new vp8softwarePipelineProperties;
  RCLCPP_DEBUG(streamer_node->get_logger(), "%sGetting props for %s%s%s", C_QUIET, C_TITLE, camera->serial.c_str(), C_RESET);
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // Get profile
  std::string profile = "";
  streamer_node->get_parameter_or<std::string>((std::string(DEFAULT_PREFIX) + "." + camera->original_serial + ".profile").c_str(), profile, profile);
  streamer_node->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + camera->serial + ".profile").c_str(), profile, profile);

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = camera->node;
  
  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera->serial, profile, camera->original_serial, "io_mode", "mmap");

  props->verify_resolution = set_property(streamer_node, camera->serial, profile, camera->original_serial, "verify_resolution", false);

  // filter
  props->format = "I420";
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera->serial, profile, camera->original_serial, "mime", default_string);

  props->brightness = set_property(streamer_node, camera->serial, profile, camera->original_serial, "brightness", 0);
  props->contrast = set_property(streamer_node, camera->serial, profile, camera->original_serial, "contrast", 0);
  props->framerate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate", 30);
  props->framerate_denominator = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate_denominator", 1);
  props->height = set_property(streamer_node, camera->serial, profile, camera->original_serial, "height", 720);
  props->width = set_property(streamer_node, camera->serial, profile, camera->original_serial, "width", 1280);

  // decoder
  default_string = "jpegdec";
  props->decoder = set_property(streamer_node, camera->serial, profile, camera->original_serial, "decoder", default_string);

  // convert
  default_string = "linear";
  props->chroma_resampler = set_property(streamer_node, camera->serial, profile, camera->original_serial, "chroma_resampler", default_string);
  default_string = "sierra-lite";
  props->dither = set_property(streamer_node, camera->serial, profile, camera->original_serial, "dither", default_string);
  default_string = "bilinear";
  props->method = set_property(streamer_node, camera->serial, profile, camera->original_serial, "method", default_string);

  // scale
  props->downscale = set_property(streamer_node, camera->serial, profile, camera->original_serial, "downscale", 1);

  // rate
  props->downrate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "downrate", 1);

  // cropper
  props->crop43 = set_property(streamer_node, camera->serial, profile, camera->original_serial, "crop43", false);

  // clock
  props->show_clock = set_property(streamer_node, camera->serial, profile, camera->original_serial, "show_clock", false);

  // encode
  props->cpu_used = set_property(streamer_node, camera->serial, profile, camera->original_serial, "cpu_used", 1);
  props->gop = set_property(streamer_node, camera->serial, profile, camera->original_serial, "gop", 1);
  props->threads = set_property(streamer_node, camera->serial, profile, camera->original_serial, "threads", 1);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera->serial, profile, camera->original_serial, "congestion_control", default_string);
  props->video_caps = "video/x-vp8";

  props->bitrate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "bitrate", 4096);

  props->do_fec = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_retransmission", false);

  return props;
}

