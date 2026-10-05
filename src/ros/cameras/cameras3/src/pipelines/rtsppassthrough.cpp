#include <string>

#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"
#include <camera_msgs/msg/camera.hpp>

#include "pipelines/properties.hpp"
#include "pipelines/pipelines.hpp"
#include "properties/common.hpp"

#include "properties/sources.hpp"
#include "properties/sinks.hpp"

#include "properties/cpufilters.hpp"

#include "properties/h26X.hpp"
#include "cameras/colors.hpp"




static void on_rtsp_pad_added(
    GstElement* source,
    GstPad* new_pad,
    gpointer user_data)
{
    GstElement* valve = GST_ELEMENT(user_data);

    GstCaps* caps = gst_pad_get_current_caps(new_pad);
    if (!caps) {
        g_printerr("RTSP pad has no caps\n");
        return;
    }

    const GstStructure* s = gst_caps_get_structure(caps, 0);

    const gchar* media = gst_structure_get_string(s, "media");
    const gchar* encoding = gst_structure_get_string(s, "encoding-name");

    g_print(
        "RTSP dynamic pad: media=%s encoding=%s\n",
        media ? media : "(none)",
        encoding ? encoding : "(none)");

    // Only accept H264 video.
    if (!media ||
        g_strcmp0(media, "video") != 0 ||
        !encoding ||
        g_ascii_strcasecmp(encoding, "H264") != 0)
    {
        g_print("Ignoring non-H264-video RTSP pad\n");
        gst_caps_unref(caps);
        return;
    }

    GstPad* sink_pad =
        gst_element_get_static_pad(valve, "sink");

    if (!sink_pad) {
        g_printerr("Could not get valve sink pad\n");
        gst_caps_unref(caps);
        return;
    }

    if (gst_pad_is_linked(sink_pad)) {
        gst_object_unref(sink_pad);
        gst_caps_unref(caps);
        return;
    }

    GstPadLinkReturn ret =
        gst_pad_link(new_pad, sink_pad);

    if (ret != GST_PAD_LINK_OK) {
        g_printerr(
            "Failed to link H264 video -> valve: %s\n",
            gst_pad_link_get_name(ret));
    }

    gst_object_unref(sink_pad);
    gst_caps_unref(caps);
}

static gboolean
on_rtsp_select_stream(
    GstElement* rtspsrc,
    guint stream_num,
    GstCaps* caps,
    gpointer user_data)
{
    GstStructure* s = gst_caps_get_structure(caps, 0);

    const gchar* media =
        gst_structure_get_string(s, "media");

    const gchar* encoding =
        gst_structure_get_string(s, "encoding-name");

    g_print(
        "RTSP stream %u: media=%s encoding=%s\n",
        stream_num,
        media ? media : "(none)",
        encoding ? encoding : "(none)");

    if (media &&
        g_strcmp0(media, "video") == 0 &&
        encoding &&
        g_ascii_strcasecmp(encoding, "H264") == 0)
    {
        return TRUE;
    }

    return FALSE;
}

/*
 * V4l camera (h264) to webrtc pipeline (direct)
 * Enforces alignment from h264 v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-h264
 */

GstElement* rtsppassthrough_pipeline(rclcpp::Node* streamer_node, const std::unique_ptr<rtsppassthroughPipelineProperties>& props)
{
  // 1. Create the elements
  std::string section = "source";
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source_rtsp = gst_element_factory_make("rtspsrc", "source_v4l");
  GstElement* source_valve = gst_element_factory_make("valve", "source_valve"); 
  GstElement* source_queue = gst_element_factory_make("queue", "source_queue");

  if (
    !gst_pipeline ||
    !source_rtsp ||
    !source_valve ||
    !source_queue
  ) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  section = "rtsp";
  GstElement* rtsp_depay = gst_element_factory_make("rtph264depay", "rtsp_depay");

  if (!rtsp_depay) {
    RCLCPP_ERROR(streamer_node->get_logger(), "%sCould not create %s%s%s elements pipeline for %s%s%s", C_FAIL, C_INPUT, section.c_str(), C_FAIL, C_TITLE, props->serial.c_str(), C_RESET);
    return nullptr;
  }

  section = "h264";
  GstElement* h264_parse = gst_element_factory_make("h264parse", "h264_parse");

  if (!h264_parse) {
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
    source_rtsp,
    source_valve,
    source_queue,
    rtsp_depay,
    h264_parse,
    webrtc_sink,
  NULL);

  // 3. Set element properties
  set_rtspsource(source_rtsp, props);
  set_queue(source_queue);

  set_h264parse(h264_parse, -1);

  set_webrtcsink(webrtc_sink, props);

  // 4. Link elements
  

  g_signal_connect(source_rtsp, "select-stream", G_CALLBACK(on_rtsp_select_stream), nullptr);
  
  g_signal_connect(source_rtsp, "pad-added", G_CALLBACK(on_rtsp_pad_added), source_valve);
  
  GstElement* next_element = source_valve;

  link_elements(streamer_node, next_element, source_queue, props->serial);

  link_elements(streamer_node, next_element, rtsp_depay, props->serial);

  link_elements(streamer_node, next_element, h264_parse, props->serial);

  link_elements(streamer_node, next_element, webrtc_sink, props->serial);

  next_element = nullptr;

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for h264passthrough pipeline or sets defaults
*/

std::unique_ptr<rtsppassthroughPipelineProperties> get_rtsppassthrough_pipeline_properties(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera)
{
  // 0. Initialize constants
  std::unique_ptr<rtsppassthroughPipelineProperties> props = std::make_unique<rtsppassthroughPipelineProperties>();
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // 1. Define default properties
  std::string default_string;

  // source
  default_string = "rtsp://user:1234@192.168.0.246:8554/profile0";
  props->url = set_property(streamer_node, camera, "url", default_string);
  default_string = "tcp";
  props->rtsp_protocol = set_property(streamer_node, camera, "rtsp_protocol", default_string);

  props->latency = set_property(streamer_node, camera, "latency", 200);

  props->non_compliant_url = set_property(streamer_node, camera, "non_compliant_url", false);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera, "congestion_control", default_string);
  props->video_caps = "video/x-h264";

  props->bitrate = set_property(streamer_node, camera, "bitrate", 4096);

  props->do_fec = set_property(streamer_node, camera, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera, "do_retransmission", false);

  // 2. Finalize props
  display_resolution(streamer_node, props, camera, 0);

  return props;
}

void set_rtsppassthrough_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<rtsppassthroughPipelineProperties>& props) {

  // Literally does nothing

  // 1. Initialize constants
  GstElement *source_valve = gst_bin_get_by_name(GST_BIN(gst_pipeline), "source_valve");

  // 2. Set properties for elements
  g_object_set(source_valve, "drop", true, NULL);

  g_object_set(source_valve, "drop", false, NULL);

  // 4. Unreference every element
  gst_object_unref(source_valve);
}

