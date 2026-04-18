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

#include "cameras/colors.hpp"

/*
 * V4l camera to webrtc pipeline
 * converts any v4l source to raw video and then encodes a format for webrtc
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1 ! decodebin ! videoconvert ! webrtcsink meta='meta, serial=(string){props->serial}' video-caps={props->video_caps}
 */


GstElement* v4lfallback_pipeline(rclcpp::Node* streamer_node, v4lfallbackPipelineProperties* props)
{
  /* 
     This creates a v4l2src to webrtc pipeline with the following structure:
     v4l2src ! capsfilter ! decodebin3 ! videoconvertscale ! scalefilter ! (clockoverlay) ! webrtcsink
     @param streamer_node pointer to the ros2 streamer node
     @param props pointer to the pipeline properties
     @return GstElement* pointer to the created GStreamer pipeline
  */

  // 0. Initialize constants 

  // Disable crop43 if it is already 4:3
  const int crop_width = crop43(props->width, props->height);
  if (crop_width == 0) {
      props->crop43 = false;
  }

  // 1. Create the elements
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* rate = (props->downrate > 1) ? gst_element_factory_make("videorate", "rater") : nullptr;
  GstElement* decode = gst_element_factory_make("decodebin3", "decoder");
  GstElement* convert = gst_element_factory_make("videoconvertscale", "converter");
  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* clock = props->show_clock ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* cropper = props->crop43 ? gst_element_factory_make("videocrop", "video-cropper") : nullptr;
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");


  if (!gst_pipeline || !source || (props->downrate > 1 && !rate) || !srcfilter || !decode || !convert || !scalefilter || (props->show_clock && !clock) || (props->crop43 && !cropper) || !webrtc 
      ) {
      RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create pipeline for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
      return nullptr;
  }

  // 2. Set element properties
  set_v4lsource(source, props->device, props->io_mode);
  set_srcfilter(srcfilter, props->mime, props->width, props->height, props->framerate, props->framerate_denominator, props->downrate, props->brightness, props->contrast);
  set_convertscale(convert, props->chroma_resampler, props->dither, props->method);
  set_scalefilter(scalefilter, props->format, props->width, props->height, props->framerate, props->framerate_denominator, props->downscale, props->downrate, props->brightness, props->contrast);
  if (props->crop43) set_crop43(cropper, crop_width, props->downscale);
  set_webrtcsink(webrtc, props->serial, props->video_caps, props->do_fec, props->do_retransmission, props->congestion_control, props->bitrate);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, srcfilter, decode, convert, scalefilter, webrtc, NULL);
  if (props->downrate > 1) gst_bin_add(GST_BIN(gst_pipeline), rate);
  if (props->crop43) gst_bin_add(GST_BIN(gst_pipeline), cropper);
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
  if (!link_elements(streamer_node, srcfilter, decode, props->serial)) return nullptr;

  g_signal_connect(decode, "pad-added", G_CALLBACK(+[](GstElement* , GstPad* new_pad, gpointer user_data) {
      GstElement* convert = static_cast<GstElement*>(user_data);
      GstPad* sink_pad = gst_element_get_static_pad(convert, "sink");
      if (sink_pad && !gst_pad_is_linked(sink_pad)) {
          gst_pad_link(new_pad, sink_pad);
      }
      if (sink_pad) gst_object_unref(sink_pad);
  }), convert);

  if (!link_elements(streamer_node, convert, scalefilter, props->serial)) return nullptr;

  // Enable crop and/or clock
  if (props->crop43 && props->show_clock) {
      if (!link_elements(streamer_node, scalefilter, cropper, props->serial)) return nullptr;
      if (!link_elements(streamer_node, cropper, clock, props->serial)) return nullptr;
      if (!link_elements(streamer_node, clock, webrtc, props->serial)) return nullptr;
  } else
  if (props->crop43) {
      if (!link_elements(streamer_node, scalefilter, cropper, props->serial)) return nullptr;
      if (!link_elements(streamer_node, cropper, webrtc, props->serial)) return nullptr;
  } else
  if (props->show_clock) {
      if (!link_elements(streamer_node, scalefilter, clock, props->serial)) return nullptr;
      if (!link_elements(streamer_node, clock, webrtc, props->serial)) return nullptr;  } 
  else {
      if (!link_elements(streamer_node, scalefilter, webrtc, props->serial)) return nullptr;
  }

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for v4l2webrtc pipeline or sets defaults
*/

v4lfallbackPipelineProperties* get_v4lfallback_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  /*
    Pulls ros2 parameters for a given camera and returns a properties struct for the v4l2webrtc pipeline creation function.
    @param streamer_node pointer to the ros2 streamer node
    @param camera pointer to the camera message containing at least the serial and node for the camera
    @return pointer to a v4l2webrtcPipelineProperties struct containing the properties for the pipeline
  */

  // 0. Initialize constants
  v4lfallbackPipelineProperties* props = new v4lfallbackPipelineProperties;
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;
  
  // 1. Define default properties
  std::string default_string;

  // source
  props->device = camera->node;

  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "io_mode", default_string);

  props->verify_resolution = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "verify_resolution", true);

  // filter
  default_string = "I420";
  props->format = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "format", default_string);
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "mime", default_string);

  props->brightness = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "brightness", 0);
  props->contrast = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "contrast", 0);
  props->framerate = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "framerate", 30);
  props->framerate_denominator = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "framerate_denominator", 1);
  props->height = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "height", 720);
  props->width = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "width", 1280);

  // convert
  default_string = "linear";
  props->chroma_resampler = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "chroma_resampler", default_string);
  default_string = "sierra-lite";
  props->dither = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "dither", default_string);
  default_string = "bilinear";
  props->method = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "method", default_string);

  // scale
  props->downscale = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "downscale", 1);

  // rate
  props->downrate = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "downrate", 1);

  // cropper
  props->crop43 = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "crop43", true);

  // clock
  props->show_clock = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "show_clock", false);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "congestion_control", default_string);
  default_string = "video/x-h264,camera->profile=constrained-baseline"; 
  props->video_caps = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "video_caps", default_string);

  props->bitrate = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "bitrate", 4096);

  props->do_fec = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera->serial, camera->profile, camera->original_serial, "do_retransmission", false);

  // 2. Finalize props
  if (props->verify_resolution) {
    if (verify_v4lresolution(props->device, &props->mime, &props->width, &props->height, &props->framerate, &props->framerate_denominator)) {
        RCLCPP_INFO(streamer_node->get_logger(), "%sInitialized pipeline: %s%s%s for %s%s%s with profile: %s%s %dx%d@%.2gfps%s", C_QUIET, C_INPUT, camera->pipeline_type.c_str(), C_QUIET, C_TITLE, props->serial.c_str(), C_QUIET, C_MODE, camera->profile.c_str(), props->width/props->downscale, props->height/props->downscale, (double) props->framerate/props->framerate_denominator/props->downrate, C_RESET);
    } else {
        RCLCPP_ERROR(streamer_node->get_logger(), "%sWrong resolution!%s Fallback pipeline: %s%s%s for %s%s%s with profile: %s%s %dx%d@%.2gfps%s", C_FAIL, C_QUIET, C_INPUT, camera->pipeline_type.c_str(), C_QUIET, C_TITLE, props->serial.c_str(), C_QUIET, C_MODE, camera->profile.c_str(), props->width/props->downscale, props->height/props->downscale, (double) props->framerate/props->framerate_denominator/props->downrate, C_RESET);
    }
  } else {
      RCLCPP_INFO(streamer_node->get_logger(), "%sInitialized pipeline: %s%s%s for %s%s%s with profile: %s%s %dx%d@%.2gfps%s", C_QUIET, C_INPUT, camera->pipeline_type.c_str(), C_QUIET, C_TITLE, props->serial.c_str(), C_QUIET, C_MODE, camera->profile.c_str(), props->width/props->downscale, props->height/props->downscale, (double) props->framerate/props->framerate_denominator/props->downrate, C_RESET);
  }

  return props;
}

