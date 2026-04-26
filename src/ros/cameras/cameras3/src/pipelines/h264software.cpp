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
#include "properties/decoders.hpp"
#include "properties/encoders.hpp"

#include "properties/h264.hpp"
#include "cameras/colors.hpp"

/*
 * V4l camera (any) decoded then encoded into x264enc
 * Enforces alignment from h264 v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-h264
 */

GstElement* h264software_pipeline(rclcpp::Node* streamer_node, h264softwarePipelineProperties* props)
{
  // 1. Create the elements
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* rate = (props->downrate > 1) ? gst_element_factory_make("videorate", "rater") : nullptr;
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* decode = (props->mime == "image/jpeg") ? gst_element_factory_make(props->decoder.c_str(), "decoder") : nullptr;
  GstElement* greyconvert = (props->greyscale) ? gst_element_factory_make("videoconvertscale", "greyconverter") : nullptr;
  GstElement* greyfilter = (props->greyscale) ? gst_element_factory_make("capsfilter", "greyfilter") : nullptr;
  GstElement* convert = gst_element_factory_make("videoconvertscale", "converter");
  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* clock = (props->show_clock) ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* cropper = (props->crop43) ? gst_element_factory_make("videocrop", "video-cropper") : nullptr;
  GstElement* encode = gst_element_factory_make("x264enc", "encoder");
  GstElement* parse = gst_element_factory_make("h264parse", "parser");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");

  if (!gst_pipeline || !source || (props->downrate > 1 && !rate) || !srcfilter || (props->mime == "image/jpeg" && !decode) || (props->greyscale && !greyconvert && !greyfilter) || !convert || !scalefilter || (props->show_clock && !clock) || (props->crop43 && !cropper) || !encode || !parse || !webrtc) {
      RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create pipeline for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
      return nullptr;
  }

  // 2. Set element properties
  set_v4lsource(source, props);
  set_srcfilter(srcfilter, props);
  if (props->mime == "image/jpeg" && props->decoder == "jpegdec") set_jpegdec(decode, props);
  if (props->greyscale) {
    set_convertscale(greyconvert, props);
    const std::string format = "GRAY8";
    set_scalefilter(greyfilter, props, format);
  }
  set_convertscale(convert, props);
  set_scalefilter(scalefilter, props);
  if (props->crop43) set_crop43(cropper, props);
  set_x264enc(encode, props);
  set_h264parse(parse, -1);
  set_webrtcsink(webrtc, props);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, srcfilter, convert, scalefilter, encode, parse, webrtc, NULL);
  if (props->downrate > 1) gst_bin_add(GST_BIN(gst_pipeline), rate);
  if (props->crop43) gst_bin_add(GST_BIN(gst_pipeline), cropper);
  if (props->show_clock) gst_bin_add(GST_BIN(gst_pipeline), clock);
  if (props->mime == "image/jpeg") gst_bin_add(GST_BIN(gst_pipeline), decode);
  if (props->greyscale) gst_bin_add_many(GST_BIN(gst_pipeline), greyconvert, greyfilter, NULL);

  // 4. Link elements

  GstElement* next_element = source;

  if (link_elements(streamer_node, next_element, rate, props->serial)) next_element = rate;
  if (link_elements(streamer_node, next_element, srcfilter, props->serial)) next_element = srcfilter;
  if (link_elements(streamer_node, next_element, decode, props->serial)) next_element = decode;
  if (link_elements(streamer_node, next_element, greyconvert, props->serial)) next_element = greyconvert;
  if (link_elements(streamer_node, next_element, greyfilter, props->serial)) next_element = greyfilter;
  if (link_elements(streamer_node, next_element, convert, props->serial)) next_element = convert;
  if (link_elements(streamer_node, next_element, scalefilter, props->serial)) next_element = scalefilter;
  if (link_elements(streamer_node, next_element, cropper, props->serial)) next_element = cropper;
  if (link_elements(streamer_node, next_element, clock, props->serial)) next_element = clock;
  if (link_elements(streamer_node, next_element, encode, props->serial)) next_element = encode;
  if (link_elements(streamer_node, next_element, parse, props->serial)) next_element = parse;
  if (link_elements(streamer_node, next_element, webrtc, props->serial)) next_element = webrtc;

  next_element = nullptr;

  return gst_pipeline;
}

/*
 * Retrieve ros2 parameters for h264software pipeline or sets defaults
*/

h264softwarePipelineProperties* get_h264software_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  // 0. Initialize constants
  h264softwarePipelineProperties* props = new h264softwarePipelineProperties;
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = camera->node;

  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera, "io_mode", default_string);

  props->verify_resolution = set_property(streamer_node, camera, "verify_resolution", true);

  // filter
  default_string = "NV12";
  props->format = set_property(streamer_node, camera, "format", default_string);
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera, "mime", default_string);

  props->brightness = set_property(streamer_node, camera, "brightness", 0);
  props->contrast = set_property(streamer_node, camera, "contrast", 0);
  props->framerate = set_property(streamer_node, camera, "framerate", 30);
  props->framerate_denominator = set_property(streamer_node, camera, "framerate_denominator", 1);
  props->height = set_property(streamer_node, camera, "height", 720);
  props->width = set_property(streamer_node, camera, "width", 1280);

  // decoder
  default_string = "jpegdec";
  props->decoder = set_property(streamer_node, camera, "decoder", default_string);
  default_string = "ifast";
  props->jpegdec_method = set_property(streamer_node, camera, "jpegdec_method", default_string);

  // greyscale
  props->greyscale = set_property(streamer_node, camera, "greyscale", false);

  // convert
  default_string = "linear";
  props->chroma_resampler = set_property(streamer_node, camera, "chroma_resampler", default_string);
  default_string = "sierra-lite";
  props->dither = set_property(streamer_node, camera, "dither", default_string);
  default_string = "bilinear";
  props->method = set_property(streamer_node, camera, "method", default_string);

  // scale
  props->downscale = set_property(streamer_node, camera, "downscale", 1);

  // rate
  props->downrate = set_property(streamer_node, camera, "downrate", 1);

  // cropper
  props->crop43 = set_property(streamer_node, camera, "crop43", true);

  // clock
  props->show_clock = set_property(streamer_node, camera, "show_clock", false);

  // encode
  props->cpu_used = set_property(streamer_node, camera, "cpu_used", 1);
  props->gop = set_property(streamer_node, camera, "gop", 1);
  props->noise = set_property(streamer_node, camera, "noise", 0);
  props->threads = set_property(streamer_node, camera, "threads", 1);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera, "congestion_control", default_string);
  props->video_caps = "video/x-h264";

  props->bitrate = set_property(streamer_node, camera, "bitrate", 4096);

  props->do_fec = set_property(streamer_node, camera, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera, "do_retransmission", false);

  // 2. Finalize props
  
  // Disable crop43 if it is already 4:3
  const int crop_width = crop43(props->width, props->height);
  if (crop_width == 0) {
      props->crop43 = false;
  }

  display_resolution(streamer_node, props, camera, crop_width);
  return props;
}

