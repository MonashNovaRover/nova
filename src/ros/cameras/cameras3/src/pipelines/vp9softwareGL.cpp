#include <string>
#include <fstream>

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
#include "properties/glfilters.hpp"
#include "properties/decoders.hpp"
#include "properties/encoders.hpp"

#include "cameras/colors.hpp"

/*
 * V4l camera (any) decoded then encoded into vp9enc
 * Enforces alignment from vp9 v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-vp9
 */

GstElement* vp9softwareGL_pipeline(rclcpp::Node* streamer_node, vp9softwareGLPipelineProperties* props)
{
  // 0. Initialize constants
  const int crop_width = (props->crop43) ? crop43(props->width, props->height) : 0;

  // 1. Create the elements
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* rate = (props->downrate > 1) ? gst_element_factory_make("videorate", "rater") : nullptr;
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* decode = (props->mime == "image/jpeg") ? gst_element_factory_make(props->decoder.c_str(), "decoder") : nullptr;
  GstElement* glupload = gst_element_factory_make("glupload", "gluploader");
  GstElement* glupconvert = gst_element_factory_make("glcolorconvert", "glupconverter");
  GstElement* glscale = (props->downscale > 1) ? gst_element_factory_make("glcolorscale", "glscaler") : nullptr;
  GstElement* gledgedetect = (props->greyscale) ? gst_element_factory_make("gleffects", "gledgedetector") : nullptr;
  GstElement* glcrop = (props->crop43) ? gst_element_factory_make("gltransformation", "glcrop") : nullptr;
  GstElement* glundistort = (props->undistort) ? gst_element_factory_make("glshader", "glundistortion") : nullptr;
  GstElement* gldownconvert = ((props->downscale > 1) || props->greyscale) ? gst_element_factory_make("glcolorconvert", "gldownconverter") : nullptr;
  GstElement* gldownload = gst_element_factory_make("gldownload", "gldownloader");
  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* clock = (props->show_clock) ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* cropper = (props->crop43) ? gst_element_factory_make("videocrop", "video-cropper") : nullptr;
  GstElement* encode = gst_element_factory_make("vp9enc", "encoder");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");

  if (
      !gst_pipeline ||
      !source ||
      (props->downrate > 1 && !rate) ||
      !srcfilter ||
      (props->mime == "image/jpeg" && !decode) ||
      !glupload ||
      !glupconvert ||
      (((props->downscale > 1) || (props->crop43)) && !glscale) ||
      (props->greyscale && !gledgedetect) ||
      ((props->crop43) && !glcrop) ||
      (props->undistort && !glundistort) ||
      (((props->downscale > 1) || props->greyscale) && !gldownconvert) ||
      !gldownload ||
      !scalefilter ||
      (props->show_clock && !clock) ||
      (props->crop43 && !cropper) ||
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
  if (props->greyscale) set_gledgedetect(gledgedetect);
  if (props->crop43) set_glcrop43(glcrop, props);
  if (props->undistort) set_glundistort(glundistort, props);
  set_scalefilter(scalefilter, props, 2*crop_width);
  set_vp9enc(encode, props);
  set_webrtcsink(webrtc, props);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline),
      source,
      srcfilter,
      glupload,
      glupconvert,
      gldownload,
      scalefilter,
      encode,
      webrtc,
      NULL);
  if (props->downrate > 1) gst_bin_add(GST_BIN(gst_pipeline), rate);
  if (props->mime == "image/jpeg") gst_bin_add(GST_BIN(gst_pipeline), decode);
  if ((props->downscale > 1) || props->crop43) gst_bin_add(GST_BIN(gst_pipeline), glscale);
  if (props->greyscale) gst_bin_add(GST_BIN(gst_pipeline), gledgedetect);
  if (props->crop43) gst_bin_add(GST_BIN(gst_pipeline), glcrop);
  if (props->undistort) gst_bin_add(GST_BIN(gst_pipeline), glundistort);
  if ((props->downscale > 1) || props->crop43) gst_bin_add(GST_BIN(gst_pipeline), gldownconvert);
  if (props->show_clock) gst_bin_add(GST_BIN(gst_pipeline), clock);

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
    if (!link_elements(streamer_node, decode, glupload, props->serial)) return nullptr;
  } else {
    if (!link_elements(streamer_node, srcfilter, glupload, props->serial)) return nullptr;
  }

  // Upload to opengl
  if (!link_elements(streamer_node, glupload, glupconvert, props->serial)) return nullptr;

  if ((props->downscale > 1) || (props->crop43)) {
    if (!link_elements(streamer_node, glupconvert, glscale, props->serial)) return nullptr;
    if (props->greyscale) {
      if (!link_elements(streamer_node, glscale, gledgedetect, props->serial)) return nullptr;
      if (props->crop43) {
        if (!link_elements(streamer_node, gledgedetect, glcrop, props->serial)) return nullptr;
        if (props->undistort) {
          if (!link_elements(streamer_node, glcrop, glundistort, props->serial)) return nullptr;
          if (!link_elements(streamer_node, glundistort, gldownconvert, props->serial)) return nullptr;
        } else {
          if (!link_elements(streamer_node, glcrop, gldownconvert, props->serial)) return nullptr;
        }
      } else if (props->undistort) {
        if (!link_elements(streamer_node, gledgedetect, glundistort, props->serial)) return nullptr;
        if (!link_elements(streamer_node, glundistort, gldownconvert, props->serial)) return nullptr;
      } else {
        if (!link_elements(streamer_node, gledgedetect, gldownconvert, props->serial)) return nullptr;
      }
    } else if (props->crop43) {
      if (!link_elements(streamer_node, glscale, glcrop, props->serial)) return nullptr;
      if (props->undistort) {
        if (!link_elements(streamer_node, glcrop, glundistort, props->serial)) return nullptr;
        if (!link_elements(streamer_node, glundistort, gldownconvert, props->serial)) return nullptr;
      } else {
        if (!link_elements(streamer_node, glcrop, gldownconvert, props->serial)) return nullptr;
      }
    } else if (props->undistort) {
      if (!link_elements(streamer_node, glscale, glundistort, props->serial)) return nullptr;
      if (!link_elements(streamer_node, glundistort, gldownconvert, props->serial)) return nullptr;
    } else {
      if (!link_elements(streamer_node, glscale, gldownconvert, props->serial)) return nullptr;
    }
  } else if (props->greyscale) {
      if (!link_elements(streamer_node, glupconvert, gledgedetect, props->serial)) return nullptr;
      else if (props->undistort) {
        if (!link_elements(streamer_node, glupconvert, glundistort, props->serial)) return nullptr;
        if (!link_elements(streamer_node, glundistort, gldownconvert, props->serial)) return nullptr;
      } else {
        if (!link_elements(streamer_node, glupconvert, gldownconvert, props->serial)) return nullptr;
      }
    } else if (props->undistort) {
      if (!link_elements(streamer_node, glupconvert, glundistort, props->serial)) return nullptr;
      if (!link_elements(streamer_node, glundistort, gldownconvert, props->serial)) return nullptr;
    } else {
      if (!link_elements(streamer_node, glupconvert, gldownconvert, props->serial)) return nullptr;
    }

  // If any gl filter was applied, there is a downconverter
  if ((props->downscale > 1) || (props->crop43) || (props->greyscale) || (props->undistort)) {
    if (!link_elements(streamer_node, gldownconvert, gldownload, props->serial)) return nullptr;
  } else {
    if (!link_elements(streamer_node, glupconvert, gldownload, props->serial)) return nullptr;
  }

  // Apply resolution scaling
  if (!link_elements(streamer_node, gldownload, scalefilter, props->serial)) return nullptr;

  // Enable crop and/or clock
  if (props->show_clock) {
      if (!link_elements(streamer_node, scalefilter, clock, props->serial)) return nullptr;
      if (!link_elements(streamer_node, clock, encode, props->serial)) return nullptr;
  } else {
      if (!link_elements(streamer_node, scalefilter, encode, props->serial)) return nullptr;
  }

  if (!link_elements(streamer_node, encode, webrtc, props->serial)) return nullptr;

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for vp9software pipeline or sets defaults
*/
vp9softwareGLPipelineProperties* get_vp9softwareGL_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  // 0. Initialize constants
  vp9softwareGLPipelineProperties* props = new vp9softwareGLPipelineProperties;
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

  // greyscale
  props->greyscale = set_property(streamer_node, camera, "greyscale", false);

  // undistort
  props->undistort_k1 = set_property(streamer_node, camera, "undistort_k1", -0.3f);
  props->undistort_k2 = set_property(streamer_node, camera, "undistort_k2", 0.1f);
  props->undistort_scale = set_property(streamer_node, camera, "undistort_scale", 1.0f);

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
  props->noise = set_property(streamer_node, camera, "noise", 0);
  props->threads = set_property(streamer_node, camera, "threads", 1);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera, "congestion_control", default_string);
  props->video_caps = "video/x-vp9";

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

