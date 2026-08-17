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

#include "properties/h26X.hpp"
#include "cameras/colors.hpp"

/*
 * V4l camera (any) decoded then encoded into h264enc
 * Enforces alignment from h264 v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-h264
 */

GstElement* h26Xsoftware_pipeline(rclcpp::Node* streamer_node, const std::unique_ptr<h26XsoftwarePipelineProperties>& props, const int h26X)
{
  
  // 1. Create the elements
  std::string section = "source";
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source_v4l = gst_element_factory_make("v4l2src", "source_v4l");
  GstElement* source_valve = gst_element_factory_make("valve", "source_valve");
  GstElement* source_rate = gst_element_factory_make("videorate", "source_rate");
  GstElement* source_filter = gst_element_factory_make("capsfilter", "source_filter");
  GstElement* source_decode = (props->mime == "image/jpeg") ? gst_element_factory_make(props->decoder.c_str(), "source_decode") : nullptr;

  if (
    !gst_pipeline ||
    !source_v4l ||
    !source_valve ||
    !source_rate ||
    !source_filter ||
    (props->mime == "image/jpeg" && !source_decode)
  ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  section = "ros";
  GstElement* ros_queue = (props->rossink) ? gst_element_factory_make("queue", "queue_ros") : nullptr;
  GstElement* ros_convert = (props->rossink) ? gst_element_factory_make("videoconvertscale", "ros_convert") : nullptr;
  GstElement* ros_filter = (props->rossink) ? gst_element_factory_make("capsfilter", "ros_filter") : nullptr;
  GstElement* ros_sink = (props->rossink) ? gst_element_factory_make("rosimagesink", "ros_sink") : nullptr;

  if (props->rossink && ( 
    !ros_queue ||
    !ros_convert ||
    !ros_filter ||
    !ros_sink)) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }
  
  section = "cpu";
  GstElement* cpu_gpu_tee = gst_element_factory_make("tee", "cpu_gpu_tee");
  GstElement* cpu_queue = gst_element_factory_make("queue", "cpu_queue");
  GstElement* cpu_valve = gst_element_factory_make("valve", "cpu_valve");
  GstElement* cpu_crop = gst_element_factory_make("videocrop", "cpu_crop");
  GstElement* cpu_convertscale = gst_element_factory_make("videoconvertscale", "cpu_convertscale");
  GstElement* cpu_gpu_selector = gst_element_factory_make("input-selector", "cpu_gpu_selector");

  if (
    !cpu_gpu_tee ||
    !cpu_queue ||
    !cpu_valve ||
    !cpu_crop ||
    !cpu_convertscale ||
    !cpu_gpu_selector
  ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  section = (std::string) "encode " + (std::string) "h26" + std::to_string(h26X);
  GstElement* encode_filter = gst_element_factory_make("capsfilter", "encode_filter");
  GstElement* encode_tee = gst_element_factory_make("tee", "encode_tee");
  GstElement* encode_queue = gst_element_factory_make("queue", "encode_queue");
  GstElement* encode_valve = gst_element_factory_make("valve", "encode_valve");
  GstElement* encode_encoder = (h26X == 5) ? gst_element_factory_make("x265enc", "encode_encoder") : gst_element_factory_make("x264enc", "encode_encoder");

  if (
    !encode_filter ||
    !encode_tee ||
    !encode_valve ||
    !encode_encoder
    ) {
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
    source_rate,
    source_filter,

    cpu_gpu_tee,
    cpu_queue,
    cpu_valve,
    cpu_crop,
    cpu_convertscale, 
    cpu_gpu_selector,

    encode_filter,
    encode_tee,
    encode_queue,
    encode_valve,
    encode_encoder,

    webrtc_sink,
  NULL);
  if (props->mime == "image/jpeg") gst_bin_add(GST_BIN(gst_pipeline), source_decode);
  if (props->rossink) gst_bin_add_many(GST_BIN(gst_pipeline), ros_queue, ros_convert, ros_filter, ros_sink, NULL);

  // 3. Set element properties
  set_v4lsource(source_v4l, props);
  set_srcfilter(source_filter, props);
  if (props->mime == "image/jpeg" && props->decoder == "jpegdec") set_jpegdec(source_decode, props);

  set_queue(cpu_queue);
  set_cpu_crop43(cpu_crop, props);
  set_convertscale(cpu_convertscale, props);

  if (props->rossink) {
    set_queue(ros_queue);
    set_convertscale(ros_convert, props);
    set_rosfilter(ros_filter, props);
    set_rostopicsink(ros_sink, props);
  }

  set_scalefilter(encode_filter, props);
  set_queue(encode_queue);
  (h26X == 5) ? set_h265enc(encode_encoder, props) : set_h264enc(encode_encoder, props);

  set_webrtcsink(webrtc_sink, props);

  // 4. Link elements
  GstElement* next_element = source_v4l;

  link_elements(streamer_node, next_element, source_valve, props->serial);
  link_elements(streamer_node, next_element, source_rate, props->serial);
  if (!link_elements(streamer_node, next_element, source_filter, props->serial)) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sWrong resolution for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }
  link_elements(streamer_node, next_element, source_decode, props->serial);

  link_elements(streamer_node, next_element, cpu_gpu_tee, props->serial);
  link_elements(streamer_node, next_element, cpu_queue, props->serial);
  link_elements(streamer_node, next_element, cpu_valve, props->serial);
  link_elements(streamer_node, next_element, cpu_crop, props->serial);
  link_elements(streamer_node, next_element, cpu_convertscale, props->serial);

  next_element = cpu_gpu_tee;

  if (props->rossink) {
    next_element = cpu_gpu_tee;
    link_elements(streamer_node, next_element, ros_queue, props->serial);
    link_elements(streamer_node, next_element, ros_convert, props->serial);
    link_elements(streamer_node, next_element, ros_filter, props->serial);
    link_elements(streamer_node, next_element, ros_sink, props->serial);
  }
 
  GstPad* cpu_source_pad = gst_element_get_static_pad(cpu_convertscale, "src");
  GstPad* cpu_sink_pad = gst_element_get_request_pad(cpu_gpu_selector, "sink_%u");
  gst_pad_link(cpu_source_pad, cpu_sink_pad);
  g_object_set(cpu_gpu_selector, "active-pad", cpu_sink_pad, NULL); // Selects which path to use
  gst_object_unref(cpu_source_pad);
  gst_object_unref(cpu_sink_pad);

  next_element = cpu_gpu_selector;
  link_elements(streamer_node, next_element, encode_filter, props->serial);
  link_elements(streamer_node, next_element, encode_tee, props->serial);
  link_elements(streamer_node, next_element, encode_queue, props->serial);
  link_elements(streamer_node, next_element, encode_valve, props->serial);
  link_elements(streamer_node, next_element, encode_encoder, props->serial);

  link_elements(streamer_node, next_element, webrtc_sink, props->serial);

  next_element = nullptr;

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for h264software pipeline or sets defaults
*/

std::unique_ptr<h26XsoftwarePipelineProperties> get_h26Xsoftware_pipeline_properties(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera, const int h26X)
{
  // 0. Initialize constants
  std::unique_ptr<h26XsoftwarePipelineProperties> props = std::make_unique<h26XsoftwarePipelineProperties>();
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = set_property(streamer_node, camera, "device", camera->node);
  
  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera, "io_mode", "mmap");

  props->brightness = set_property(streamer_node, camera, "brightness", -1);
  props->contrast = set_property(streamer_node, camera, "contrast", -1);
  props->saturation = set_property(streamer_node, camera, "saturation", -1);
  props->gain = set_property(streamer_node, camera, "gain", -1);
  props->sharpness = set_property(streamer_node, camera, "sharpness", -1);

  // filter
  props->format = (h26X == 5) ? "I420_10LE" : "I420";
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera, "mime", default_string);

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
  
  // convert
  default_string = "cubic";
  props->chroma_resampler = set_property(streamer_node, camera, "chroma_resampler", default_string);
  default_string = "sierra-lite";
  props->dither = set_property(streamer_node, camera, "dither", default_string);
  default_string = "mitchell";
  props->method = set_property(streamer_node, camera, "method", default_string);

  // scale
  props->downscale = set_property(streamer_node, camera, "downscale", 1);

  // rate
  props->downrate = set_property(streamer_node, camera, "downrate", 1);

  // cropper
  props->zoom = set_property(streamer_node, camera, "zoom", 1.0);
  props->zoom_longitude = set_property(streamer_node, camera, "zoom_longitude", 0.0);
  props->zoom_latitude = set_property(streamer_node, camera, "zoom_latitude", 0.0);

  props->crop43 = set_property(streamer_node, camera, "crop43", true);

  // encode
  props->cpu_used = set_property(streamer_node, camera, "cpu_used", 0);
  props->deadline = set_property(streamer_node, camera, "deadline", 1);
  props->gop = set_property(streamer_node, camera, "gop", 2);
  props->encoder_denoise = set_property(streamer_node, camera, "encoder_denoise", 6);
  props->threads = set_property(streamer_node, camera, "threads", 1);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera, "congestion_control", default_string);
  props->video_caps = (h26X == 5) ? "video/x-h265" : "video/x-h264";
  props->bitrate = set_property(streamer_node, camera, "bitrate", 1024);

  props->do_fec = set_property(streamer_node, camera, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera, "do_retransmission", false);

  // 2. Finalize props
  const int crop_width = props->crop43 ? crop43(props->width, props->height, props->zoom): 0;
  display_resolution(streamer_node, props, camera, crop_width);

  return props;
}

void set_h26Xsoftware_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<h26XsoftwarePipelineProperties>& props) {

  // 1. Initialize constants
  GstElement* source_v4l = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_v4l");
  GstElement* source_filter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_filter");
  GstPad* source_source_pad = gst_element_get_static_pad(source_filter, "src");
  GstCaps* source_caps = gst_pad_get_current_caps(source_source_pad);
  GstElement* source_decode = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_decode");

  GstElement* cpu_crop = gst_bin_get_by_name(GST_BIN(gst_pipeline), "cpu_crop");

  GstElement* encode_filter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "encode_filter");
  GstPad* encode_source_pad = gst_element_get_static_pad(encode_filter, "src");
  GstCaps* encode_caps = gst_pad_get_current_caps(encode_source_pad);
  GstElement* encode_encoder = gst_bin_get_by_name(GST_BIN(gst_pipeline), "encode_encoder");
  GstElementFactory* encode_factory = gst_element_get_factory(encode_encoder);

  GstElement* cpu_valve = gst_bin_get_by_name(GST_BIN(gst_pipeline), "cpu_valve");
  GstElement* source_valve = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_valve");

  GstElement* cpu_gpu_selector = gst_bin_get_by_name(GST_BIN(gst_pipeline), "cpu_gpu_selector");
  GstElement* cpu_convertscale = gst_bin_get_by_name(GST_BIN(gst_pipeline), "cpu_convertscale");
  GstPad* cpu_source_pad = gst_element_get_static_pad(cpu_convertscale, "src");
  GstPad* cpu_sink_pad = gst_pad_get_peer(cpu_source_pad);
  
  // 2. Set properties for elements
  g_object_set(source_valve, "drop", true, NULL);
  const int h26X = ((std::string) gst_plugin_feature_get_name(GST_PLUGIN_FEATURE(encode_factory)) == "x265enc") ? 5 : 4;

  const GstStructure* encode_str = gst_caps_get_structure(encode_caps, 0);
  const GstStructure* source_str = gst_caps_get_structure(source_caps, 0);

  if (source_v4l) {
    set_v4lsource(source_v4l, props);
    gst_object_unref(source_v4l);
  }

  // Do not change width or height if using vp8
  if (h26X == 4) {
    gst_structure_get_int(encode_str, "width", &props->width);
    gst_structure_get_int(encode_str, "height", &props->height);
  }

  if (source_filter) {
    if (h26X == 4) {
      gst_structure_get_int(source_str, "width", &props->width);
      gst_structure_get_int(source_str, "height", &props->height);
    }
    set_srcfilter(source_filter, props);
    if (h26X == 4) {
      gst_structure_get_int(encode_str, "width", &props->width);
      gst_structure_get_int(encode_str, "height", &props->height);
    }
    gst_object_unref(source_filter);
  }  if (source_decode) {
    if (props->mime == "image/jpeg") set_jpegdec(source_decode, props);
    gst_object_unref(source_decode);
  }

  if (cpu_crop) {
    if (h26X == 4) {
      gst_structure_get_int(source_str, "width", &props->width);
      gst_structure_get_int(source_str, "height", &props->height);
    }
    set_cpu_crop43(cpu_crop, props);
    if (h26X == 4) {
      gst_structure_get_int(encode_str, "width", &props->width);
      gst_structure_get_int(encode_str, "height", &props->height);
    }
    gst_object_unref(cpu_crop);
  }

  if (encode_filter) { 
    if (h26X == 5) set_scalefilter(encode_filter, props);
    gst_object_unref(encode_filter);
  }
  if (encode_encoder) {
    (h26X == 5) ? set_h265enc(encode_encoder, props) : set_h264enc(encode_encoder, props);
    gst_object_unref(encode_encoder);
  }

  // 3. Swap input now, avoids race condition
  g_object_set(source_valve, "drop", false, NULL);

  // 4. Unreference every element
  gst_object_unref(source_source_pad);
  gst_caps_unref(source_caps);
  gst_object_unref(encode_source_pad);
  gst_caps_unref(encode_caps);
  gst_object_unref(cpu_source_pad);
  gst_object_unref(cpu_sink_pad);
  gst_object_unref(cpu_gpu_selector);
  gst_object_unref(cpu_convertscale);
  gst_object_unref(source_valve);
  gst_object_unref(cpu_valve);
}
