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
 * V4l camera to webrtc pipeline + ros topic
 * converts any v4l source to raw video and then encodes a format for webrtc
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1 ! decodebin ! videoconvert ! webrtcsink meta='meta, serial=(string){props->serial}' video-caps={props->video_caps}
 */


GstElement* v4lrostopic_pipeline(rclcpp::Node* streamer_node, v4lrostopicPipelineProperties* props)
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
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* decode = gst_element_factory_make("decodebin3", "decoder");

  GstElement* tee = gst_element_factory_make("tee", "tee");
  GstElement* queue1 = gst_element_factory_make("queue", "queue1");
  GstElement* rosconvert = gst_element_factory_make("videoconvertscale", "rosconverter");
  GstElement* rosfilter = gst_element_factory_make("capsfilter", "rosfilter");
  GstElement* rossink = gst_element_factory_make("rosimagesink", "rossink");

  GstElement* queue2 = gst_element_factory_make("queue", "queue2");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");


  if (!gst_pipeline || !source || !srcfilter || !decode || !tee || !queue1 || !rosconvert || !rosfilter || !rossink || !queue2 || !webrtc 
      ) {
      RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create pipeline for %s%s%s", C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
      return nullptr;
  }

  // 2. Set element properties
  set_v4lsource(source, props);
  set_srcfilter(srcfilter, props);
  set_queue(queue1);
  set_rosfilter(rosfilter, props);
  set_rostopicsink(rossink, props);
  set_queue(queue2);
  set_webrtcsink(webrtc, props);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, srcfilter, decode, tee, queue1, rosconvert, rosfilter, rossink, queue2, webrtc, NULL);

  // 4. Link elements
  if (!link_elements(streamer_node, source, srcfilter, props->serial)) return nullptr;

  // Convert to raw
  if (!link_elements(streamer_node, srcfilter, decode, props->serial)) return nullptr;

  // Link to tee
  g_signal_connect(decode, "pad-added", G_CALLBACK(+[](GstElement* , GstPad* new_pad, gpointer user_data) {
      GstElement* tee = static_cast<GstElement*>(user_data);
      GstPad* sink_pad = gst_element_get_static_pad(tee, "sink");
      if (sink_pad && !gst_pad_is_linked(sink_pad)) {
          gst_pad_link(new_pad, sink_pad);
      }
      if (sink_pad) gst_object_unref(sink_pad);
  }), tee);

  // connect to ros topic
  if (!link_elements(streamer_node, tee, queue1, props->serial)) return nullptr;
  if (!link_elements(streamer_node, queue1, rosconvert, props->serial)) return nullptr;
  if (!link_elements(streamer_node, rosconvert, rosfilter, props->serial)) return nullptr;
  if (!link_elements(streamer_node, rosfilter, rossink, props->serial)) return nullptr;
  
  if (!link_elements(streamer_node, tee, queue2, props->serial)) return nullptr;
  if (!link_elements(streamer_node, queue2, webrtc, props->serial)) return nullptr;

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for v4l2webrtc pipeline or sets defaults
*/

v4lrostopicPipelineProperties* get_v4lrostopic_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  /*
    Pulls ros2 parameters for a given camera and returns a properties struct for the v4l2webrtc pipeline creation function.
    @param streamer_node pointer to the ros2 streamer node
    @param camera pointer to the camera message containing at least the serial and node for the camera
    @return pointer to a v4l2webrtcPipelineProperties struct containing the properties for the pipeline
  */

  // 0. Initialize constants
  v4lrostopicPipelineProperties* props = new v4lrostopicPipelineProperties;
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;
  
  // 1. Define default properties
  std::string default_string;

  // source
  props->device = camera->node;

  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera, "io_mode", default_string);

  props->verify_resolution = set_property(streamer_node, camera, "verify_resolution", false);

  // filter
  default_string = "BGR";
  props->format = set_property(streamer_node, camera, "format", default_string);
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera, "mime", default_string);

  props->brightness = set_property(streamer_node, camera, "brightness", 0);
  props->contrast = set_property(streamer_node, camera, "contrast", 0);
  props->framerate = set_property(streamer_node, camera, "framerate", 30);
  props->framerate_denominator = set_property(streamer_node, camera, "framerate_denominator", 1);
  props->height = set_property(streamer_node, camera, "height", 720);
  props->width = set_property(streamer_node, camera, "width", 1280);

  // rossink
  default_string = camera->serial;
  props->ros_topic = set_property(streamer_node, camera, "ros_topic", default_string);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera, "congestion_control", default_string);
  default_string = "video/x-vp8";
  props->video_caps = set_property(streamer_node, camera, "video_caps", default_string);

  props->bitrate = set_property(streamer_node, camera, "bitrate", 4096);

  props->do_fec = set_property(streamer_node, camera, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera, "do_retransmission", false);

  // 2. Finalize props
  display_resolution(streamer_node, props, camera, 0);

  return props;
}

