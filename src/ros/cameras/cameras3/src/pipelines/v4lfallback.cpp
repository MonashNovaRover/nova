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
  std::string section = "source";
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source_v4l = gst_element_factory_make("v4l2src", "source_v4l");
  GstElement* source_valve = gst_element_factory_make("valve", "source_valve");
  GstElement* source_rate = gst_element_factory_make("videorate", "source_rate");
  GstElement* source_filter = gst_element_factory_make("capsfilter", "source_filter"); 
  GstElement* source_decode = gst_element_factory_make("decodebin3", "source_decode");

  if (
    !gst_pipeline ||
    !source_v4l ||
    !source_valve ||
    !source_rate ||
    !source_filter ||
    !source_decode
  ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  section = "cpu";
  GstElement* cpu_crop = gst_element_factory_make("videocrop", "cpu_crop");
  GstElement* cpu_convertscale = gst_element_factory_make("videoconvertscale", "cpu_convertscale");
  GstElement* cpu_filter = gst_element_factory_make("capsfilter", "cpu_filter");
  GstElement* cpu_clock = props->show_clock ? gst_element_factory_make("clockoverlay", "cpu_clock") : nullptr;

  if (
    !cpu_crop ||
    !cpu_convertscale ||
    !cpu_filter ||
    (props->show_clock && !cpu_clock)
    ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create pipeline for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
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
    source_decode,

    cpu_crop,
    cpu_convertscale,
    cpu_filter,

    webrtc_sink,
  NULL);
  if (props->show_clock) gst_bin_add(GST_BIN(gst_pipeline), cpu_clock);

  // 3. Set element properties
  set_v4lsource(source_v4l, props);
  set_srcfilter(source_filter, props);

  set_cpu_crop43(cpu_crop, props);
  set_convertscale(cpu_convertscale, props);
  set_scalefilter(cpu_filter, props);

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

  g_signal_connect(source_decode, "pad-added", G_CALLBACK(+[](GstElement* , GstPad* new_pad, gpointer user_data) {
      GstElement* cpu_crop = static_cast<GstElement*>(user_data);
      GstPad* sink_pad = gst_element_get_static_pad(cpu_crop, "sink");
      if (sink_pad && !gst_pad_is_linked(sink_pad)) {
          gst_pad_link(new_pad, sink_pad);
      }
      if (sink_pad) gst_object_unref(sink_pad);
  }), cpu_crop);

  next_element = cpu_crop;

  link_elements(streamer_node, next_element, cpu_convertscale, props->serial);
  link_elements(streamer_node, next_element, cpu_filter, props->serial);
  link_elements(streamer_node, next_element, cpu_clock, props->serial);
  link_elements(streamer_node, next_element, webrtc_sink, props->serial);

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
  props->device = set_property(streamer_node, camera, "device", camera->node);

  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera, "io_mode", default_string);

  props->brightness = set_property(streamer_node, camera, "brightness", 0);
  props->contrast = set_property(streamer_node, camera, "contrast", 50);
  props->saturation = set_property(streamer_node, camera, "saturation", 64);
  props->gain = set_property(streamer_node, camera, "gain", 0);
  props->sharpness = set_property(streamer_node, camera, "sharpness", 50);

  // filter
  default_string = "I420";
  props->format = set_property(streamer_node, camera, "format", default_string);
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera, "mime", default_string);

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
  props->zoom = set_property(streamer_node, camera, "zoom", 1.0);
  props->zoom_longitude = set_property(streamer_node, camera, "zoom_longitude", 0.0);
  props->zoom_latitude = set_property(streamer_node, camera, "zoom_latitude", 0.0);

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
  const int crop_width = props->crop43 ? crop43(props->width, props->height, props->zoom): 0;
  display_resolution(streamer_node, props, camera, crop_width);

  return props;
}

void set_v4lfallback_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<v4lfallbackPipelineProperties>& props) {
  const int crop_width = (props->crop43) ? crop43(props->width, props->height, props->zoom) : 0;

  // 1. Initialize constants
  GstElement *source_valve = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_valve");
  GstElement* source_filter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_filter");
  GstElement* cpu_filter = gst_bin_get_by_name(GST_BIN(gst_pipeline), "cpu_filter");
  GstElement* cpu_crop = gst_bin_get_by_name(GST_BIN(gst_pipeline), "cpu_crop");

  // 2. Set properties for elements
  g_object_set(source_valve, "drop", true, NULL);

  if (source_filter) {
    set_srcfilter(source_filter, props);
    gst_object_unref(source_filter);
  }

  if (cpu_filter) { 
    set_scalefilter(cpu_filter, props);
    gst_object_unref(cpu_filter);
  }

  if (cpu_crop) {
    set_cpu_crop43(cpu_crop, props);
    gst_object_unref(cpu_crop);
  }

  g_object_set(source_valve, "drop", false, NULL);

  // 4. Unreference every element
  gst_object_unref(source_valve);
}
