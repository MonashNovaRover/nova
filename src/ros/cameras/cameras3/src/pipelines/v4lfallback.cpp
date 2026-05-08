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


GstElement* v4lfallback_pipeline(rclcpp::Node* streamer_node, const std::unique_ptr<v4lfallbackPipelineProperties>& props)
{
  /* 
     This creates a v4l2src to webrtc pipeline with the following structure:
     v4l2src ! capsfilter ! decodebin3 ! videoconvertscale ! scalefilter ! (clockoverlay) ! webrtcsink
     @param streamer_node pointer to the ros2 streamer node
     @param props pointer to the pipeline properties
     @return GstElement* pointer to the created GStreamer pipeline
  */

  // 1. Create the elements
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* valve = gst_element_factory_make("valve", "video-valve");
  GstElement* rate = gst_element_factory_make("videorate", "rater");
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter"); 
  GstElement* decode = gst_element_factory_make("decodebin3", "decoder");
  GstElement* convert = gst_element_factory_make("videoconvertscale", "converter");
  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* clock = props->show_clock ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* cropper = gst_element_factory_make("videocrop", "video-cropper");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");


  if (!gst_pipeline ||
    !source ||
    !valve ||
    !rate ||
    !srcfilter ||
    !decode ||
    !convert ||
    !scalefilter ||
    (props->show_clock && !clock) ||
    !cropper ||
    !webrtc 
    ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create pipeline for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  // 2. Set element properties
  set_v4lsource(source, props);
  set_srcfilter(srcfilter, props);
  set_convertscale(convert, props);
  set_scalefilter(scalefilter, props);
  if (props->crop43) set_cpu_crop43(cropper, props);
  set_webrtcsink(webrtc, props);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline),
    source,
    valve,
    rate,
    srcfilter,
    decode,
    convert,
    cropper,
    scalefilter,
    webrtc,
  NULL);
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

  g_signal_connect(decode, "pad-added", G_CALLBACK(+[](GstElement* , GstPad* new_pad, gpointer user_data) {
      GstElement* convert = static_cast<GstElement*>(user_data);
      GstPad* sink_pad = gst_element_get_static_pad(convert, "sink");
      if (sink_pad && !gst_pad_is_linked(sink_pad)) {
          gst_pad_link(new_pad, sink_pad);
      }
      if (sink_pad) gst_object_unref(sink_pad);
  }), convert);

  next_element = convert;

  if (link_elements(streamer_node, next_element, scalefilter, props->serial)) next_element = scalefilter;
  if (link_elements(streamer_node, next_element, cropper, props->serial)) next_element = cropper;
  if (link_elements(streamer_node, next_element, clock, props->serial)) next_element = clock;
  link_elements(streamer_node, next_element, webrtc, props->serial);

  next_element = nullptr;

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for v4l2webrtc pipeline or sets defaults
*/

std::unique_ptr<v4lfallbackPipelineProperties> get_v4lfallback_pipeline_properties(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera)
{
  /*
    Pulls ros2 parameters for a given camera and returns a properties struct for the v4l2webrtc pipeline creation function.
    @param streamer_node pointer to the ros2 streamer node
    @param camera pointer to the camera message containing at least the serial and node for the camera
    @return pointer to a v4l2webrtcPipelineProperties struct containing the properties for the pipeline
  */

  // 0. Initialize constants
  std::unique_ptr<v4lfallbackPipelineProperties> props = std::make_unique<v4lfallbackPipelineProperties>();
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
  default_string = "I420";
  props->format = set_property(streamer_node, camera, "format", default_string);
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera, "mime", default_string);

  props->brightness = set_property(streamer_node, camera, "brightness", 0);
  props->contrast = set_property(streamer_node, camera, "contrast", 0);
  props->framerate = set_property(streamer_node, camera, "framerate", 30);
  props->framerate_denominator = set_property(streamer_node, camera, "framerate_denominator", 1);
  props->height = set_property(streamer_node, camera, "height", 720);
  props->width = set_property(streamer_node, camera, "width", 1280);

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

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera, "congestion_control", default_string);
  default_string = "video/x-vp8"; 
  props->video_caps = set_property(streamer_node, camera, "video_caps", default_string);

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

void set_v4lfallback_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<v4lfallbackPipelineProperties>& props) {
  const int crop_width = (props->crop43) ? crop43(props->width, props->height) : 0;
  if (crop_width == 0) props->crop43 = false;

  // 1. Find the elements
  GstElement* srcfilter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "srcfilter");
  GstElement* scalefilter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "scalefilter");
  GstElement* cropper = gst_bin_get_by_name(GST_BIN(gst_pipeline), "video-cropper");

  // 2. Set properties for elements
  if (srcfilter) {
    set_srcfilter(srcfilter, props);
    gst_object_unref(srcfilter);
  }

  if (scalefilter) { 
    set_scalefilter(scalefilter, props);
    gst_object_unref(scalefilter);
  }

  if (cropper) {
    if (props->crop43) set_cpu_crop43(cropper, props);
    else set_no_cpu_crop43(cropper);
    gst_object_unref(cropper);
  }
}
