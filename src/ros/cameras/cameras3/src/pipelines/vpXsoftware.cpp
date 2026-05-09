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
#include "properties/glfilters.hpp"

#include "cameras/colors.hpp"

/*
 * V4l camera (any) decoded then encoded into vpXenc
 * Enforces alignment from vpX v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-vpX
 */

GstElement* vpXsoftware_pipeline(rclcpp::Node* streamer_node, const std::unique_ptr<vpXsoftwarePipelineProperties>& props, const int vpX)
{
  // 1. Create the elements
  std::string section = "source";
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source_v4l = gst_element_factory_make("v4l2src", "source_v4l");
  GstElement* source_valve = gst_element_factory_make("valve", "source_valve"); 
  GstElement* source_filter = gst_element_factory_make("capsfilter", "source_filter");
  GstElement* source_rate = gst_element_factory_make("videorate", "source_rate");
  GstElement* source_rate_filter = gst_element_factory_make("capsfilter", "source_rate_filter");
  GstElement* source_decode = (props->mime == "image/jpeg") ? gst_element_factory_make(props->decoder.c_str(), "source_decode") : nullptr;

  if (
    !gst_pipeline ||
    !source_v4l ||
    !source_valve || 
    !source_filter ||
    !source_rate ||
    !source_rate_filter ||
    (props->mime == "image/jpeg" && !source_decode)
  ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  section = "ros";
  GstElement* ros_tee = (props->rossink) ? gst_element_factory_make("tee", "ros_tee") : nullptr;
  GstElement* ros_queue = (props->rossink) ? gst_element_factory_make("queue", "queue_ros") : nullptr;
  GstElement* ros_convert = (props->rossink) ? gst_element_factory_make("videoconvertscale", "ros_convert") : nullptr;
  GstElement* ros_filter = (props->rossink) ? gst_element_factory_make("capsfilter", "ros_filter") : nullptr;
  GstElement* ros_sink = (props->rossink) ? gst_element_factory_make("rosimagesink", "ros_sink") : nullptr;

  if (props->rossink && (
    !ros_tee ||
    !ros_queue ||
    !ros_convert ||
    !ros_filter ||
    !ros_sink
  )) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }
  
  section = "cpu";
  GstElement* cpu_gpu_tee = gst_element_factory_make("tee", "cpu_gpu_tee");
  GstElement* cpu_queue = gst_element_factory_make("queue", "cpu_queue");
  GstElement* cpu_valve = gst_element_factory_make("valve", "cpu_valve");
  GstElement* cpu_crop = gst_element_factory_make("videocrop", "cpu_crop");
  GstElement* cpu_grey_convert = gst_element_factory_make("videoconvertscale", "cpu_grey_convert");
  GstElement* cpu_grey_filter = gst_element_factory_make("capsfilter", "cpu_grey_filter");
  GstElement* cpu_convertscale = gst_element_factory_make("videoconvertscale", "cpu_convertscale");

  if (
    !cpu_gpu_tee ||
    !cpu_queue ||
    !cpu_valve ||
    !cpu_crop ||
    !cpu_grey_convert ||
    !cpu_grey_filter ||
    !cpu_convertscale
  ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  section = "gpu";
  GstElement* gpu_queue = gst_element_factory_make("queue", "gpu_queue");
  GstElement* gpu_valve = gst_element_factory_make("valve", "gpu_valve");
  GstElement* gpu_upload = gst_element_factory_make("glupload", "gpu_upload");
  GstElement* gpu_convertup = gst_element_factory_make("glcolorconvert", "gpu_convertup");
  GstElement* gpu_scale = gst_element_factory_make("glcolorscale", "gpu_scale");
  GstElement* gpu_crop = gst_element_factory_make("gltransformation", "gpu_crop");
  GstElement* gpu_shaders = gst_element_factory_make("glshader", "gpu_shaders");
  GstElement* gpu_convertdown = gst_element_factory_make("glcolorconvert", "gpu_convertdown");
  GstElement* gpu_download = gst_element_factory_make("gldownload", "gpu_download");
  GstElement* cpu_gpu_selector = gst_element_factory_make("input-selector", "cpu_gpu_selector");

  if (
    !gpu_queue ||
    !gpu_valve ||
    !gpu_upload ||
    !gpu_convertup ||
    !gpu_scale ||
    !gpu_crop ||
    !gpu_shaders ||
    !gpu_convertdown ||
    !gpu_download ||
    !cpu_gpu_selector
  ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  section = (std::string) "encode " + (std::string) "vp9";
  GstElement* encode_tee = gst_element_factory_make("tee", "encode_tee");
  GstElement* encode_queue = gst_element_factory_make("queue", "encode_queue");
  GstElement* encode_valve = gst_element_factory_make("valve", "encode_valve");
  GstElement* encode_filter = gst_element_factory_make("capsfilter", "encode_filter");
  GstElement* encode_vp9 = gst_element_factory_make("vp9enc", "encode_vp9");
  GstElement* encode_selector = gst_element_factory_make("input-selector", "encode_selector");

  if (
    !encode_tee ||
    !encode_valve ||
    !encode_filter ||
    !encode_vp9 ||
    !encode_selector
    ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  section = "sink";
  GstElement* sink_webrtc = gst_element_factory_make("webrtcsink", "sink_webrtc");

  if (
    !sink_webrtc
  ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  // 2. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline),
    source_v4l,
    source_valve, 
    source_filter,
    source_rate,
    source_rate_filter,

    cpu_gpu_tee,
    cpu_queue,
    cpu_valve,
    cpu_crop,
    cpu_grey_convert,
    cpu_grey_filter, 
    cpu_convertscale, 
    
    gpu_queue,
    gpu_valve,
    gpu_upload,
    gpu_convertup,
    gpu_scale,
    gpu_crop,
    gpu_shaders,
    gpu_convertdown,
    gpu_download,
    cpu_gpu_selector,

    encode_tee,
    encode_queue,
    encode_valve,
    encode_filter,
    encode_vp9,
    encode_selector,

    sink_webrtc,
  NULL);
  if (props->mime == "image/jpeg") gst_bin_add(GST_BIN(gst_pipeline), source_decode);
  if (props->rossink) gst_bin_add_many(GST_BIN(gst_pipeline), ros_tee, ros_queue, ros_convert, ros_filter, ros_sink, NULL);

  // 3. Set element properties
  set_v4lsource(source_v4l, props);
  set_srcfilter(source_filter, props);
  set_ratefilter(source_rate_filter, props);
  if (props->mime == "image/jpeg" && props->decoder == "jpegdec") set_jpegdec(source_decode, props);

  if (props->rossink) {
    set_queue(ros_queue);
    set_convertscale(ros_convert, props);
    set_rosfilter(ros_filter, props);
    set_rostopicsink(ros_sink, props);
  }

  set_queue(cpu_queue);
  g_object_set(cpu_valve, "drop", props->use_gl, NULL);
  if (props->crop43) set_cpu_crop43(cpu_crop, props);
  else set_no_cpu_crop43(cpu_crop);
  if (props->greyscale) set_cpu_grey_filter(cpu_grey_filter, props);
  else set_no_cpu_grey_filter(cpu_grey_filter);
  set_convertscale(cpu_grey_convert, props);
  set_convertscale(cpu_convertscale, props);

  set_queue(gpu_queue);
  g_object_set(gpu_valve, "drop", !props->use_gl, NULL);
  if (props->crop43) set_glcrop43(gpu_crop, props);
  set_glshaders(gpu_shaders, props);

  set_queue(encode_queue);
  set_scalefilter(encode_filter, props);
  if (vpX == 9) set_vp9enc(encode_vp9, props);

  set_webrtcsink(sink_webrtc, props);

  // 4. Link elements
  GstElement* next_element = source_v4l;

  link_elements(streamer_node, next_element, source_valve, props->serial); 
  if (!link_elements(streamer_node, next_element, source_filter, props->serial)) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sWrong resolution for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }
  link_elements(streamer_node, next_element, source_rate, props->serial);
  link_elements(streamer_node, next_element, source_rate_filter, props->serial);
  link_elements(streamer_node, next_element, source_decode, props->serial);

  link_elements(streamer_node, next_element, ros_tee, props->serial);
  link_elements(streamer_node, next_element, ros_queue, props->serial);
  link_elements(streamer_node, next_element, ros_convert, props->serial);
  link_elements(streamer_node, next_element, ros_filter, props->serial);
  link_elements(streamer_node, next_element, ros_sink, props->serial);

  link_elements(streamer_node, next_element, cpu_gpu_tee, props->serial);
  link_elements(streamer_node, next_element, cpu_queue, props->serial);
  link_elements(streamer_node, next_element, cpu_valve, props->serial);
  link_elements(streamer_node, next_element, cpu_crop, props->serial);
  link_elements(streamer_node, next_element, cpu_grey_convert, props->serial);
  link_elements(streamer_node, next_element, cpu_grey_filter, props->serial);
  link_elements(streamer_node, next_element, cpu_convertscale, props->serial);

  next_element = cpu_gpu_tee;
  link_elements(streamer_node, next_element, gpu_queue, props->serial);
  link_elements(streamer_node, next_element, gpu_valve, props->serial);
  link_elements(streamer_node, next_element, gpu_upload, props->serial);
  link_elements(streamer_node, next_element, gpu_convertup, props->serial);
  link_elements(streamer_node, next_element, gpu_scale, props->serial);
  link_elements(streamer_node, next_element, gpu_crop, props->serial);
  link_elements(streamer_node, next_element, gpu_shaders, props->serial);
  link_elements(streamer_node, next_element, gpu_convertdown, props->serial);
  link_elements(streamer_node, next_element, gpu_download, props->serial);
 
  GstPad* cpu_source_pad = gst_element_get_static_pad(cpu_convertscale, "src");
  GstPad* gpu_source_pad = gst_element_get_static_pad(gpu_download, "src");
  GstPad* cpu_sink_pad = gst_element_get_request_pad(cpu_gpu_selector, "sink_%u");
  GstPad* gpu_sink_pad = gst_element_get_request_pad(cpu_gpu_selector, "sink_%u");
  gst_pad_link(cpu_source_pad, cpu_sink_pad);
  gst_pad_link(gpu_source_pad, gpu_sink_pad);
  if (props->use_gl) g_object_set(cpu_gpu_selector, "active-pad", gpu_sink_pad, NULL);
  else g_object_set(cpu_gpu_selector, "active-pad", cpu_sink_pad, NULL);
  gst_object_unref(cpu_source_pad);
  gst_object_unref(gpu_source_pad);
  gst_object_unref(cpu_sink_pad);
  gst_object_unref(gpu_sink_pad);

  next_element = cpu_gpu_selector;
  link_elements(streamer_node, next_element, encode_tee, props->serial);
  link_elements(streamer_node, next_element, encode_queue, props->serial);
  link_elements(streamer_node, next_element, encode_valve, props->serial);
  link_elements(streamer_node, next_element, encode_filter, props->serial);
  link_elements(streamer_node, next_element, encode_vp9, props->serial);
  //link_elements(streamer_node, next_element, encode_selector, props->serial);

  link_elements(streamer_node, next_element, sink_webrtc, props->serial);

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

  // use opengl
  props->use_gl = set_property(streamer_node, camera, "use_gl", false);
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

  // gl filters
  props->denoise_factor = set_property(streamer_node, camera, "denoise_factor", 0.5f);
  props->denoise_sigma = set_property(streamer_node, camera, "denoise_sigma", 2.0f);
  props->denoise_threshold = set_property(streamer_node, camera, "denoise_threshold", 0.1f);
  props->denoise_radius = set_property(streamer_node, camera, "denoise_radius", 3);
  props->sharpen_radius = set_property(streamer_node, camera, "sharpen_radius", 1.0f);
  props->sharpen_strength = set_property(streamer_node, camera, "sharpen_strength", 2.0f);
  props->undistort_k1 = set_property(streamer_node, camera, "undistort_k1", -0.3f);
  props->undistort_k2 = set_property(streamer_node, camera, "undistort_k2", 0.1f);
  props->undistort_scale = set_property(streamer_node, camera, "undistort_scale", 1.0f);

  props->denoise = set_property(streamer_node, camera, "denoise", false);
  props->sharpen = set_property(streamer_node, camera, "sharpen", false);
  props->undistort = set_property(streamer_node, camera, "undistort", false);
  
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
  props->noise = set_property(streamer_node, camera, "noise", (props->use_gl) ? 0 : 6);
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

  // 0. Initialize constants
  GstElement* source_filter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_filter");
  GstElement* source_rate_filter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_rate_filter");
  GstElement* source_decode = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_decode");

  GstElement* cpu_crop = gst_bin_get_by_name(GST_BIN(gst_pipeline), "cpu_crop");
  GstElement* cpu_grey_filter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "cpu_grey_filter");

  GstElement* gpu_crop = gst_bin_get_by_name(GST_BIN(gst_pipeline), "gpu_crop");
  GstElement* gpu_shaders = gst_bin_get_by_name(GST_BIN(gst_pipeline), "gpu_shaders");

  GstElement* encode_filter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "encode_filter");
  GstElement* encode_vp9 = gst_bin_get_by_name(GST_BIN(gst_pipeline), "encode_vp9");

  GstElement* cpu_valve = gst_bin_get_by_name(GST_BIN(gst_pipeline), "cpu_valve");
  GstElement* gpu_valve = gst_bin_get_by_name(GST_BIN(gst_pipeline), "gpu_valve");

  GstElement* cpu_gpu_selector = gst_bin_get_by_name(GST_BIN(gst_pipeline), "cpu_gpu_selector");
  GstElement* cpu_convertscale = gst_bin_get_by_name(GST_BIN(gst_pipeline), "cpu_convertscale");
  GstElement* gpu_download = gst_bin_get_by_name(GST_BIN(gst_pipeline), "gpu_download");
  GstPad* cpu_source_pad = gst_element_get_static_pad(cpu_convertscale, "src");
  GstPad* gpu_source_pad = gst_element_get_static_pad(gpu_download, "src");
  GstPad* cpu_sink_pad = gst_pad_get_peer(cpu_source_pad);
  GstPad* gpu_sink_pad = gst_pad_get_peer(gpu_source_pad);

  GstElement *source_valve = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_valve");
  g_object_set(source_valve, "drop", true, NULL);

  // 2. Set properties for elements
  if (source_filter) {
    set_srcfilter(source_filter, props);
    gst_object_unref(source_filter);
  }
  if (source_rate_filter) {
    set_ratefilter(source_rate_filter, props);
    gst_object_unref(source_rate_filter);
  }
  if (source_decode) {
    if (props->mime == "image/jpeg") set_jpegdec(source_decode, props);
    gst_object_unref(source_decode);
  }

  if (cpu_crop) {
    if (props->crop43) set_cpu_crop43(cpu_crop, props);
    else set_no_cpu_crop43(cpu_crop);
    gst_object_unref(cpu_crop);
  }
  if (cpu_grey_filter) {
    if (props->greyscale) set_cpu_grey_filter(cpu_grey_filter, props);
    else set_no_cpu_grey_filter(cpu_grey_filter);
    gst_object_unref(cpu_grey_filter);
  }

  if (gpu_crop) {
    if (props->crop43) set_glcrop43(gpu_crop, props);
    else set_no_glcrop43(gpu_crop);
    gst_object_unref(gpu_crop);
  }
  if (gpu_shaders) {
    set_glshaders(gpu_shaders, props);
    gst_object_unref(gpu_shaders);
  }

  if (encode_filter) { 
    if (vpX == 9) set_scalefilter(encode_filter, props);
    gst_object_unref(encode_filter);
  }
  if (encode_vp9) {
    (vpX == 9) ? set_vp9enc(encode_vp9, props) : set_vp8enc(encode_vp9, props);
    gst_object_unref(encode_vp9);
  }

  // 3. Swap input now, avoids race condition
  g_object_set(cpu_valve, "drop", props->use_gl, NULL);
  g_object_set(gpu_valve, "drop", !props->use_gl, NULL);
  if (props->use_gl) {
    g_object_set(cpu_gpu_selector, "active-pad", gpu_sink_pad, NULL);
    gst_pad_send_event(cpu_sink_pad, gst_event_new_flush_start());
    gst_pad_send_event(cpu_sink_pad, gst_event_new_flush_stop(TRUE));
  }
  else {
    g_object_set(cpu_gpu_selector, "active-pad", cpu_sink_pad, NULL);
    gst_pad_send_event(gpu_sink_pad, gst_event_new_flush_start());
    gst_pad_send_event(gpu_sink_pad, gst_event_new_flush_stop(TRUE));
  }
  g_object_set(source_valve, "drop", false, NULL);

  // 4. Unreference every element
  gst_object_unref(cpu_source_pad);
  gst_object_unref(gpu_source_pad);
  gst_object_unref(cpu_sink_pad);
  gst_object_unref(gpu_sink_pad);
  gst_object_unref(cpu_gpu_selector);
  gst_object_unref(cpu_convertscale);
  gst_object_unref(gpu_download);
  gst_object_unref(source_valve);
  gst_object_unref(cpu_valve);
  gst_object_unref(gpu_valve);
}
