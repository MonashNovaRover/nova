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

#include "cameras/colors.hpp"

/*
 * V4l camera (any) decoded then encoded into vpXenc
 * Enforces alignment from vpX v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-vpX
 */

GstElement* vpXsoftware_pipeline(rclcpp::Node* streamer_node, const std::unique_ptr<vpXsoftwarePipelineProperties>& props, const int vpX)
{
  // 1. Create the elements
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* valve = gst_element_factory_make("valve", "video-valve");
  GstElement* rate = gst_element_factory_make("videorate", "rate");
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* decode = (props->mime == "image/jpeg") ? gst_element_factory_make(props->decoder.c_str(), "decoder") : nullptr;

  GstElement* tee = (props->rossink) ? gst_element_factory_make("tee", "tee") : nullptr;
  GstElement* queue_ros = (props->rossink) ? gst_element_factory_make("queue", "queue_ros") : nullptr;
  GstElement* rosconvert = (props->rossink) ? gst_element_factory_make("videoconvertscale", "rosconverter") : nullptr;
  GstElement* rosfilter = (props->rossink) ? gst_element_factory_make("capsfilter", "rosfilter") : nullptr;
  GstElement* rossink = (props->rossink) ? gst_element_factory_make("rosimagesink", "rossink") : nullptr;
  GstElement* queue_webrtc = (props->rossink) ? gst_element_factory_make("queue", "queue_webrtc") : nullptr;

  GstElement* greyconvert = gst_element_factory_make("videoconvertscale", "greyconverter");
  GstElement* greyfilter = gst_element_factory_make("capsfilter", "greyfilter");
  GstElement* convert = gst_element_factory_make("videoconvertscale", "converter");
  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* clock = (props->show_clock) ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* cropper = gst_element_factory_make("videocrop", "video-cropper");
  GstElement* encode = (vpX == 9) ? gst_element_factory_make("vp9enc", "encoder") : gst_element_factory_make("vp8enc", "encoder");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");

  if (!gst_pipeline ||
      !source ||
      !valve ||
      !rate ||
      !srcfilter ||
      (props->mime == "image/jpeg" && !decode) ||
      (props->rossink && !tee && !queue_ros && !rosconvert && !rosfilter && !rossink && !queue_webrtc) ||
      !greyconvert ||
      !greyfilter ||
      !convert ||
      !scalefilter  ||
      (props->show_clock && !clock) ||
      !cropper ||
      !encode ||
      !webrtc
      ) {
      RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create pipeline for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
      return nullptr;
  }
  
  // 2. Set element properties
  set_v4lsource(source, props);
  set_srcfilter(srcfilter, props);
  if (props->mime == "image/jpeg" && props->decoder == "jpegdec") set_jpegdec(decode, props);
  if (props->rossink) {
    set_queue(queue_ros);
    set_convertscale(rosconvert, props);
    set_rosfilter(rosfilter, props);
    set_rostopicsink(rossink, props);
    set_queue(queue_webrtc);
  }
  if (props->greyscale) set_greyfilter(greyfilter);
  else set_no_greyfilter(greyfilter);
  set_convertscale(convert, props);
  set_scalefilter(scalefilter, props);
  if (props->crop43) set_crop43(cropper, props);
  (vpX == 9) ? set_vp9enc(encode, props) : set_vp8enc(encode, props);
  set_webrtcsink(webrtc, props);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline),
      source,
      valve,
      rate, 
      srcfilter,
      greyconvert,
      greyfilter,
      convert,
      scalefilter,
      cropper,
      encode,
      webrtc,
      NULL);
  if (props->mime == "image/jpeg") gst_bin_add(GST_BIN(gst_pipeline), decode);
  if (props->rossink) gst_bin_add_many(GST_BIN(gst_pipeline), tee, queue_ros, rosconvert, rosfilter, rossink, queue_webrtc, NULL);
  if (props->show_clock) gst_bin_add(GST_BIN(gst_pipeline), clock);

  // 4. Link elements
  
  GstElement* next_element = source;

  if (link_elements(streamer_node, next_element, valve, props->serial)) next_element = valve;
  if (link_elements(streamer_node, next_element, rate, props->serial)) next_element = rate;
  if (link_elements(streamer_node, next_element, srcfilter, props->serial)) next_element = srcfilter;
  else {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sWrong resolution for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }
  if (link_elements(streamer_node, next_element, decode, props->serial)) next_element = decode;

  if (link_elements(streamer_node, next_element, tee, props->serial)) next_element = tee;
  if (link_elements(streamer_node, next_element, queue_ros, props->serial)) next_element = queue_ros;
  if (link_elements(streamer_node, next_element, rosconvert, props->serial)) next_element = rosconvert;
  if (link_elements(streamer_node, next_element, rosfilter, props->serial)) next_element = rosfilter;
  if (link_elements(streamer_node, next_element, rossink, props->serial)) next_element = tee;
  if (link_elements(streamer_node, next_element, queue_webrtc, props->serial)) next_element = queue_webrtc;

  if (link_elements(streamer_node, next_element, greyconvert, props->serial)) next_element = greyconvert;
  if (link_elements(streamer_node, next_element, greyfilter, props->serial)) next_element = greyfilter;
  if (link_elements(streamer_node, next_element, convert, props->serial)) next_element = convert;
  if (link_elements(streamer_node, next_element, scalefilter, props->serial)) next_element = scalefilter;
  if (link_elements(streamer_node, next_element, cropper, props->serial)) next_element = cropper;
  if (link_elements(streamer_node, next_element, clock, props->serial)) next_element = clock;
  if (link_elements(streamer_node, next_element, encode, props->serial)) next_element = encode;
  link_elements(streamer_node, next_element, webrtc, props->serial);

  next_element = nullptr;

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for vpXsoftware pipeline or sets defaults
*/

std::unique_ptr<vpXsoftwarePipelineProperties> get_vpXsoftware_pipeline_properties(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera, const int vpX)
{
  // 0. Initialize constants
  std::unique_ptr<vpXsoftwarePipelineProperties> props = std::make_unique<vpXsoftwarePipelineProperties>();
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = camera->node;
  
  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera, "io_mode", "mmap");

  // filter
  props->format = "I420";
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

  // rossink
  default_string = "BGR";
  props->ros_format = set_property(streamer_node, camera, "ros_format", default_string);
  default_string = camera->serial;
  props->ros_topic = set_property(streamer_node, camera, "ros_topic", default_string);

  props->rossink = set_property(streamer_node, camera, "rossink", false);

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
  props->deadline = set_property(streamer_node, camera, "deadline", 1);
  props->gop = set_property(streamer_node, camera, "gop", 1);
  props->noise = set_property(streamer_node, camera, "noise", 6);
  props->threads = set_property(streamer_node, camera, "threads", 1);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera, "congestion_control", default_string);
  props->video_caps = (vpX == 9) ? "video/x-vp9" : "video/x-vp8";

  props->bitrate = set_property(streamer_node, camera, "bitrate", 4096);

  props->do_fec = set_property(streamer_node, camera, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera, "do_retransmission", false);

  // 2. Finalize props
 
  // Disable crop43 if it is already 4:3
  const int crop_width = (props->crop43) ? crop43(props->width, props->height) : 0;
  if (crop_width == 0) props->crop43 = false;

  display_resolution(streamer_node, props, camera, crop_width);

  return props;
}

void set_vpXsoftware_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<vpXsoftwarePipelineProperties>& props, const int vpX) {
  const int crop_width = (props->crop43) ? crop43(props->width, props->height) : 0;
  if (crop_width == 0) props->crop43 = false;

  // 1. Find the elements
  GstElement* srcfilter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "srcfilter");
  GstElement* decode = gst_bin_get_by_name(GST_BIN(gst_pipeline), "decoder");
  GstElement* greyfilter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "greyfilter");
  GstElement* scalefilter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "scalefilter");
  GstElement* cropper = gst_bin_get_by_name(GST_BIN(gst_pipeline), "video-cropper");
  GstElement* encode = gst_bin_get_by_name(GST_BIN(gst_pipeline), "encoder");

  // 2. Set properties for elements
  if (srcfilter) {
    if (vpX == 9) set_srcfilter(srcfilter, props);
    gst_object_unref(srcfilter);
  }

  if (decode) {
    if (props->mime == "image/jpeg") set_jpegdec(decode, props);
    gst_object_unref(decode);
  }
  if (greyfilter) {
    if (props->greyscale) set_greyfilter(greyfilter);
    else set_no_greyfilter(greyfilter);
    gst_object_unref(greyfilter);
  }

  if (scalefilter) { 
    if (vpX == 9) set_scalefilter(scalefilter, props);
    gst_object_unref(scalefilter);
  }

  if (cropper) {
    if (vpX == 9 && props->crop43) set_crop43(cropper, props);
    else set_no_crop43(cropper);
    gst_object_unref(cropper);
  }


  if (encode) {
    (vpX == 9) ? set_vp9enc(encode, props) : set_vp8enc(encode, props);
    gst_object_unref(encode);
  }
}
