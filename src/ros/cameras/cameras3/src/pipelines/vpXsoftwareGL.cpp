#include <string>

#include <gst/gst.h>
#include <gst/gl/gl.h>
#include <gst/gl/gstglcontext.h>
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
 * V4l camera (any) decoded then encoded into vpXenc
 * Enforces alignment from vpX v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-vpX
 */

GstElement* vpXsoftwareGL_pipeline(rclcpp::Node* streamer_node, const std::unique_ptr<vpXsoftwareGLPipelineProperties>& props, const int vpX)
{
  // 0. Initialize constants
  const int crop_width = (props->crop43) ? crop43(props->width, props->height) : 0;

  // 1. Create the elements
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* valve = gst_element_factory_make("valve", "video-valve");
  GstElement* rate = (props->downrate > 1) ? gst_element_factory_make("videorate", "rater") : nullptr;
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* decode = (props->mime == "image/jpeg") ? gst_element_factory_make(props->decoder.c_str(), "decoder") : nullptr;

  GstElement* tee = (props->rossink) ? gst_element_factory_make("tee", "tee") : nullptr;
  GstElement* queue_ros = (props->rossink) ? gst_element_factory_make("queue", "queue_ros") : nullptr;
  GstElement* rosconvert = (props->rossink) ? gst_element_factory_make("videoconvertscale", "rosconverter") : nullptr;
  GstElement* rosfilter = (props->rossink) ? gst_element_factory_make("capsfilter", "rosfilter") : nullptr;
  GstElement* rossink = (props->rossink) ? gst_element_factory_make("rosimagesink", "rossink") : nullptr;
  GstElement* queue_upload = gst_element_factory_make("queue", "queue_upload");

  GstElement* glupload = gst_element_factory_make("glupload", "gluploader");
  GstElement* glupconvert = gst_element_factory_make("glcolorconvert", "glupconverter");
  GstElement* glscale = ((props->downscale > 1) || (props->crop43)) ? gst_element_factory_make("glcolorscale", "glscaler") : nullptr;
  GstElement* glcrop = (props->crop43) ? gst_element_factory_make("gltransformation", "glcrop") : nullptr;
  GstElement* glgreyscale = (props->greyscale) ? gst_element_factory_make("glcolorbalance", "glgreyscale") : nullptr;
  GstElement* gldenoise = (props->denoise) ? gst_element_factory_make("glshader", "gldenoise") : nullptr;
  GstElement* gledgedetect = (props->edgedetect) ? gst_element_factory_make("glshader", "gledgedetect") : nullptr;
  GstElement* glundistort = (props->undistort) ? gst_element_factory_make("glshader", "glundistortion") : nullptr;
  GstElement* gldownconvert = ((props->downscale > 1) || props->crop43 || props->greyscale || props->denoise || props->edgedetect || props->undistort) ? gst_element_factory_make("glcolorconvert", "gldownconverter") : nullptr;
  GstElement* gldownload = gst_element_factory_make("gldownload", "gldownloader");
  GstElement* queue_download = gst_element_factory_make("queue", "queue_download");

  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* clock = (props->show_clock) ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* encode = (vpX == 9) ? gst_element_factory_make("vp9enc", "encoder") : gst_element_factory_make("vp8enc", "encoder");  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");

  if (
    !gst_pipeline ||
    !source ||
    !valve ||
    (props->downrate > 1 && !rate) ||
    !srcfilter ||
    (props->mime == "image/jpeg" && !decode) ||
    (props->rossink && !tee && !queue_ros && !rosconvert && !rosfilter && !rossink) ||
    !queue_upload ||
    !glupload ||
    !glupconvert ||
    (((props->downscale > 1) || (props->crop43)) && !glscale) ||
    (props->crop43 && !glcrop) ||
    (props->greyscale && !glgreyscale) ||
    (props->denoise && !gldenoise) ||
    (props->edgedetect && !gledgedetect) ||
    (props->undistort && !glundistort) ||
    (((props->downscale > 1) || props->crop43 || props->greyscale || props->denoise || props->edgedetect || props->undistort)  && !gldownconvert) ||
    !gldownload ||
    !queue_download ||

    !scalefilter ||
    (props->show_clock && !clock) ||
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
  }
  set_queue(queue_upload);
  if (props->crop43) set_glcrop43(glcrop, props);
  if (props->greyscale) set_glgreyscale(glgreyscale);
  if (props->denoise) set_gldenoise(gldenoise, props);
  if (props->edgedetect) set_gledgedetect(gledgedetect, props);
  if (props->undistort) set_glundistort(glundistort, props);
  set_queue(queue_download);

  set_scalefilter(scalefilter, props, crop_width*2);
  (vpX == 9) ? set_vp9enc(encode, props) : set_vp8enc(encode, props);
  set_webrtcsink(webrtc, props);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline),
      source,
      valve,
      srcfilter,
      queue_upload,
      glupload,
      glupconvert,
      gldownload,
      queue_download,
      scalefilter,
      encode,
      webrtc,
      NULL);
  if (props->downrate > 1) gst_bin_add(GST_BIN(gst_pipeline), rate);
  if (props->mime == "image/jpeg") gst_bin_add(GST_BIN(gst_pipeline), decode);
  if (props->rossink) gst_bin_add_many(GST_BIN(gst_pipeline), tee, queue_ros, rosconvert, rosfilter, rossink, NULL);
  if ((props->downscale > 1) || props->crop43) gst_bin_add(GST_BIN(gst_pipeline), glscale);
  if (props->crop43) gst_bin_add(GST_BIN(gst_pipeline), glcrop);
  if (props->greyscale) gst_bin_add(GST_BIN(gst_pipeline), glgreyscale);
  if (props->denoise) gst_bin_add(GST_BIN(gst_pipeline), gldenoise);
  if (props->edgedetect) gst_bin_add(GST_BIN(gst_pipeline), gledgedetect);
  if (props->undistort) gst_bin_add(GST_BIN(gst_pipeline), glundistort);
  if ((props->downscale > 1) || props->crop43 || props->greyscale || props->denoise || props->edgedetect || props->undistort)  gst_bin_add(GST_BIN(gst_pipeline), gldownconvert);
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
  if (link_elements(streamer_node, next_element, queue_upload, props->serial)) next_element = queue_upload;

  if (link_elements(streamer_node, next_element, glupload, props->serial)) next_element = glupload;
  if (link_elements(streamer_node, next_element, glupconvert, props->serial)) next_element = glupconvert;
  if (link_elements(streamer_node, next_element, glscale, props->serial)) next_element = glscale;
  if (link_elements(streamer_node, next_element, glcrop, props->serial)) next_element = glcrop;
  if (link_elements(streamer_node, next_element, glgreyscale, props->serial)) next_element = glgreyscale;
  if (link_elements(streamer_node, next_element, glundistort, props->serial)) next_element = glundistort;
  if (link_elements(streamer_node, next_element, gldenoise, props->serial)) next_element = gldenoise;
  if (link_elements(streamer_node, next_element, gledgedetect, props->serial)) next_element = gledgedetect;
  if (link_elements(streamer_node, next_element, gldownconvert, props->serial)) next_element = gldownconvert;
  if (link_elements(streamer_node, next_element, gldownload, props->serial)) next_element = gldownload;
  if (link_elements(streamer_node, next_element, queue_download, props->serial)) next_element = queue_download;

  if (link_elements(streamer_node, next_element, scalefilter, props->serial)) next_element = scalefilter;
  if (link_elements(streamer_node, next_element, clock, props->serial)) next_element = clock;
  if (link_elements(streamer_node, next_element, encode, props->serial)) next_element = encode;
  link_elements(streamer_node, next_element, webrtc, props->serial);

  next_element = nullptr;

  return gst_pipeline;
}

/*
 * Retrieve ros2 parameters for vpXsoftware pipeline or sets defaults
*/
std::unique_ptr<vpXsoftwareGLPipelineProperties> get_vpXsoftwareGL_pipeline_properties(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera, const int vpX)
{
  // 0. Initialize constants
  std::unique_ptr<vpXsoftwareGLPipelineProperties> props = std::make_unique<vpXsoftwareGLPipelineProperties>();
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = camera->node;
  
  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera, "io_mode", default_string);

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
  props->denoise_factor = set_property(streamer_node, camera, "denoise_factor", 1.0f);
  props->denoise_sigma = set_property(streamer_node, camera, "denoise_sigma", 2.0f);
  props->denoise_threshold = set_property(streamer_node, camera, "denoise_threshold", 0.1f);
  props->denoise_radius = set_property(streamer_node, camera, "denoise_radius", 3);
  props->edgedetect_factor = set_property(streamer_node, camera, "edgedetect_factor", 2.0f);
  props->undistort_k1 = set_property(streamer_node, camera, "undistort_k1", -0.3f);
  props->undistort_k2 = set_property(streamer_node, camera, "undistort_k2", 0.1f);
  props->undistort_scale = set_property(streamer_node, camera, "undistort_scale", 1.0f);

  props->denoise = set_property(streamer_node, camera, "denoise", false);
  props->edgedetect = set_property(streamer_node, camera, "edgedetect", false);
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

