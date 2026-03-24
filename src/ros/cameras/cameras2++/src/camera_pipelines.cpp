#include <optional>
#include <string>

#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"

#include <camera_msgs/msg/camera.hpp>

#include "cameras/pipeline.hpp"

#include <yaml-cpp/yaml.h>

static bool is_plugin_available(const std::string& plugin_name) {
  GstElementFactory* factory = gst_element_factory_find(plugin_name.c_str());
  if (factory != nullptr) {
      gst_object_unref(factory);
      return true;
  } 
  return false;
}

/*
 * V4l camera to webrtc pipeline
 * converts any v4l source to raw video and then encodes a format for webrtc
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1 ! decodebin ! videoconvert ! webrtcsink meta='meta, serial=(string){props->serial}' video-caps={props->video_caps}
 */


GstElement* v4l2webrtc_pipeline(rclcpp::Node* streamer_node, v4l2webrtcPipelineProperties* props)
{
  /* 
     This creates a v4l2src to webrtc pipeline with the following structure:
     v4l2src ! capsfilter ! decodebin -> videoconvert ! (clockoverlay) ! webrtcsink
     @param streamer_node pointer to the ros2 streamer node
     @param props pointer to the pipeline properties
     @return GstElement* pointer to the created GStreamer pipeline
  */

  // create the elements
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* filter = gst_element_factory_make("capsfilter", "filter");
  GstElement* decode = gst_element_factory_make("decodebin", "decoder");
  GstElement* convert = gst_element_factory_make("videoconvert", "converter");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");
  GstElement* clock = props->show_clock ? gst_element_factory_make("clockoverlay", "clock") : nullptr;

  if (!gst_pipeline || !source || !filter || !decode || !convert || !webrtc
      || (props->show_clock && !clock) 
      ) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }
  RCLCPP_INFO(streamer_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate);
  
  // set element properties
  g_object_set(source,
      "device", props->device.c_str(),
      "io-mode", (
        props->io_mode == "rw" ? 1 :
        props->io_mode == "mmap" ? 2 :
        props->io_mode == "userptr" ? 3 :
        props->io_mode == "dmabuf" ? 4 :
        props->io_mode == "dmabuf-import" ? 5 :0),
      NULL);

  GstCaps *caps = gst_caps_new_simple(
      props->mime.c_str(),
      "width", G_TYPE_INT, props->width,
      "height", G_TYPE_INT, props->height,
      "framerate", GST_TYPE_FRACTION, props->framerate, 1,
      "brightness", G_TYPE_INT, props->brightness,
      "contrast", G_TYPE_INT,  props->contrast,
      NULL);
  g_object_set(filter, "caps", caps, NULL);
  gst_caps_unref(caps);

  GstStructure *meta = gst_structure_new("meta", "serial", G_TYPE_STRING, props->serial.c_str(), NULL); 
  GstCaps *webrtc_caps = gst_caps_from_string(props->video_caps.c_str());
  g_object_set(webrtc,
      "do-fec", props->do_fec,
      "do-retransmission", props->do_retransmission,
      "congestion-control", (
        props->congestion_control == "disabled" ? 0 :
        props->congestion_control == "homegrown" ? 1 :
        props->congestion_control == "gcc" ? 2 : -1),
      "meta", meta,
      "video-caps", webrtc_caps,
      NULL);
  gst_caps_unref(webrtc_caps);
  gst_structure_free(meta);

  // add elements to pipeline and link
  gst_bin_add_many(GST_BIN(gst_pipeline), source, filter, decode, convert, webrtc, NULL);

  g_signal_connect(decode, "pad-added", G_CALLBACK(+[](GstElement* , GstPad* new_pad, gpointer user_data) {
      GstElement* convert = static_cast<GstElement*>(user_data);
      GstPad* sink_pad = gst_element_get_static_pad(convert, "sink");
      if (sink_pad && !gst_pad_is_linked(sink_pad)) {
          gst_pad_link(new_pad, sink_pad);
      }
      if (sink_pad) gst_object_unref(sink_pad);
  }), convert);

  //g_signal_connect(webrtc, "encoder-setup", G_CALLBACK(encoder_setup), NULL);

  bool ret = true;

  ret = gst_element_link(source, filter) ? ret : false;
  ret = gst_element_link(filter, decode) ? ret : false;

  if (props->show_clock) {
      gst_bin_add(GST_BIN(gst_pipeline), clock);
      ret = gst_element_link(convert, clock) ? ret : false;
      ret = gst_element_link(clock, webrtc) ? ret : false;
  } else {
      ret = gst_element_link(convert, webrtc) ? ret : false;
  }

  if (!ret) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not link elements of pipeline for %s", props->serial.c_str());
      return nullptr;
  }

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for v4l2webrtc pipeline or sets defaults
*/

v4l2webrtcPipelineProperties* get_v4l2webrtc_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  /*
    Pulls ros2 parameters for a given camera and returns a properties struct for the v4l2webrtc pipeline creation function.
    @param streamer_node pointer to the ros2 streamer node
    @param camera pointer to the camera message containing at least the serial and node for the camera
    @return pointer to a v4l2webrtcPipelineProperties struct containing the properties for the pipeline
  */
  v4l2webrtcPipelineProperties* props = new v4l2webrtcPipelineProperties; 

  std::map<std::string, rclcpp::Parameter> serial_params;

  RCLCPP_INFO(streamer_node->get_logger(), "Getting props for %s", camera->serial.c_str());
  props->serial = camera->serial;
  props->node = camera->node;

  // Read yaml file
  YAML::Node config = YAML::LoadFile("/home/nova/nova/src/ros/cameras/cameras2++/params/streamer.yaml");
  YAML::Node param = config["camera_streamer"]["ros__parameters"][std::string(PIPELINE_PREFIX)][camera->serial];
  YAML::Node profile = config["camera_streamer"]["ros__parameters"]["profiles"][param["profile"].as<std::string>("default")];

  // override any defaults with params
  std::string camera_prefix = std::string(PIPELINE_PREFIX) + "." + camera->serial;
  props->device = param["device"] ? param["device"].as<std::string>() : profile["device"].as<std::string>(props->node);
  props->width = param["width"] ? param["width"].as<int>() : profile["width"].as<int>(1280);
  props->height = param["height"] ? param["height"].as<int>() : profile["height"].as<int>(720);
  props->framerate = param["framerate"] ? param["framerate"].as<int>() : profile["framerate"].as<int>(30);
  props->brightness = param["brightness"] ? param["brightness"].as<int>() : profile["brightness"].as<int>(0);
  props->contrast = param["contrast"] ? param["contrast"].as<int>() : profile["contrast"].as<int>(0);
  props->mime = param["mime"] ? param["mime"].as<std::string>() : profile["mime"].as<std::string>("image/jpeg");
  props->congestion_control = param["congestion_control"] ? param["congestion_control"].as<std::string>() : profile["congestion_control"].as<std::string>("gcc");
  props->do_fec = param["do_fec"] ? param["do_fec"].as<bool>() : profile["do_fec"].as<bool>(false);
  props->do_retransmission = param["do_retransmission"] ? param["do_retransmission"].as<bool>() : profile["do_retransmission"].as<bool>(false);
  props->video_caps = param["video_caps"] ? param["video_caps"].as<std::string>() : profile["video_caps"].as<std::string>("video/x-h264,profile=constrained-baseline");
  props->show_clock = param["show_clock"] ? param["show_clock"].as<bool>() : profile["show_clock"].as<bool>(false);
  props->io_mode = param["io_mode"] ? param["io_mode"].as<std::string>() : profile["io_mode"].as<std::string>("mmap");

  return props;
}

/*
 * V4l camera (h264) to webrtc pipeline (direct)
 * Enforces alignment from h264 v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-h264
 */
GstElement* h264passthrough_pipeline(rclcpp::Node* streamer_node, h264passthroughPipelineProperties* props)
{
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* filter = gst_element_factory_make("capsfilter", "filter");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");

  if (!gst_pipeline || !source || !filter || !webrtc) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }
  RCLCPP_INFO(streamer_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate);
  g_object_set(source,
      "device", props->device.c_str(),
      "io-mode", 4,
      NULL);

  GstCaps *caps = gst_caps_new_simple(
      "video/x-h264",
      "width", G_TYPE_INT, props->width,
      "height", G_TYPE_INT, props->height,
      "framerate", GST_TYPE_FRACTION, props->framerate, 1,
      "brightness", G_TYPE_INT, props->brightness,
      "contrast", G_TYPE_INT,  props->contrast,
      "alignment", G_TYPE_STRING, "au",
      NULL);
  g_object_set(filter, "caps", caps, NULL);
  gst_caps_unref(caps);

  GstStructure *meta = gst_structure_new("meta", "serial", G_TYPE_STRING, props->serial.c_str(), NULL);
  const char *video_codec = "video/x-h264";
  GstCaps *webrtc_caps = gst_caps_from_string(video_codec);
  g_object_set(webrtc,
      "do-fec", props->do_fec,
      "do-retransmission", props->do_retransmission,
      "congestion-control", (
        props->congestion_control == "disabled" ? 0 :
        props->congestion_control == "homegrown" ? 1 :
        props->congestion_control == "gcc" ? 2 :
        2),
      "meta", meta,
      "video-caps", webrtc_caps,
      NULL);
  gst_caps_unref(webrtc_caps);
  gst_structure_free(meta);
    
  gst_bin_add_many(GST_BIN(gst_pipeline), source, filter, webrtc, NULL);

  bool ret = true;

  ret = gst_element_link(source, filter) ? ret : false;
  if (props->payload_quirk) {
    // Apply patch for gc2093
    GstElement* payload = gst_element_factory_make("rtph264pay", "payloader");
    GstElement* depayload = gst_element_factory_make("rtph264depay", "depayloader");
    g_object_set(payload,
        "aggregate-mode", 1,
        "config-interval", 1,
        NULL);
    gst_bin_add_many(GST_BIN(gst_pipeline), payload, depayload, NULL);
    ret = gst_element_link(filter, payload) ? ret : false;
    ret = gst_element_link(payload, depayload) ? ret : false;
    ret = gst_element_link(depayload, webrtc) ? ret: false;
  } else {
    GstElement* parse = gst_element_factory_make("h264parse", "parser");
    gst_bin_add(GST_BIN(gst_pipeline), parse);
    g_object_set(parse, "config-interval", -1, NULL);
    ret = gst_element_link(filter, parse) ? ret : false;
    ret = gst_element_link(parse, webrtc) ? ret : false;
  }

  if (!ret) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not link elements of pipeline for %s", props->serial.c_str());
      return nullptr;
  }

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for h264passthrough pipeline or sets defaults
*/

h264passthroughPipelineProperties* get_h264passthrough_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  h264passthroughPipelineProperties* props = new h264passthroughPipelineProperties; 

  std::map<std::string, rclcpp::Parameter> serial_params;

  RCLCPP_INFO(streamer_node->get_logger(), "Getting props for %s", camera->serial.c_str());
  props->serial = camera->serial;
  props->node = camera->node;

  // Read yaml file
  YAML::Node config = YAML::LoadFile("/home/nova/nova/src/ros/cameras/cameras2++/params/streamer.yaml");
  YAML::Node param = config["camera_streamer"]["ros__parameters"][std::string(PIPELINE_PREFIX)][camera->serial];
  YAML::Node profile = config["camera_streamer"]["ros__parameters"]["profiles"][param["profile"].as<std::string>("default")];

  // override any defaults with params
  std::string camera_prefix = std::string(PIPELINE_PREFIX) + "." + camera->serial;
  props->device = param["device"] ? param["device"].as<std::string>() : profile["device"].as<std::string>(props->node);
  props->width = param["width"] ? param["width"].as<int>() : profile["width"].as<int>(1280);
  props->height = param["height"] ? param["height"].as<int>() : profile["height"].as<int>(720);
  props->framerate = param["framerate"] ? param["framerate"].as<int>() : profile["framerate"].as<int>(30);
  props->brightness = param["brightness"] ? param["brightness"].as<int>() : profile["brightness"].as<int>(0);
  props->contrast = param["contrast"] ? param["contrast"].as<int>() : profile["contrast"].as<int>(0);
  props->congestion_control = param["congestion_control"] ? param["congestion_control"].as<std::string>() : profile["congestion_control"].as<std::string>("gcc");
  props->do_fec = param["do_fec"] ? param["do_fec"].as<bool>() : profile["do_fec"].as<bool>(false);
  props->do_retransmission = param["do_retransmission"] ? param["do_retransmission"].as<bool>() : profile["do_retransmission"].as<bool>(false);
  props->payload_quirk = param["payload_quirk"] ? param["payload_quirk"].as<bool>() : profile["payload_quirk"].as<bool>(false);

  return props;
}

/*
 * V4l camera (any) decoded then encoded into x264enc
 * Enforces alignment from h264 v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-h264
 */

GstElement* h264software_pipeline(rclcpp::Node* streamer_node, h264softwarePipelineProperties* props)
{
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* filter = gst_element_factory_make("capsfilter", "filter");
  GstElement* encode = gst_element_factory_make("x264enc", "encoder");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");

  if (!gst_pipeline || !source || !filter || !encode || !webrtc) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }
  RCLCPP_INFO(streamer_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate);

  g_object_set(source,
      "device", props->device.c_str(),
      "io-mode", (
        props->io_mode == "rw" ? 1 :
        props->io_mode == "mmap" ? 2 :
        props->io_mode == "userptr" ? 3 :
        props->io_mode == "dmabuf" ? 4 :
        props->io_mode == "dmabuf-import" ? 5 :0),
      NULL);

  GstCaps *caps = gst_caps_new_simple(
      props->mime.c_str(),
      "width", G_TYPE_INT, props->width,
      "height", G_TYPE_INT, props->height,
      "framerate", GST_TYPE_FRACTION, props->framerate, 1,
      "brightness", G_TYPE_INT, props->brightness,
      "contrast", G_TYPE_INT,  props->contrast,
      NULL);
  g_object_set(filter, "caps", caps, NULL); 
  gst_caps_unref(caps);

  g_object_set(encode,
    "tune", (
      props->tune == "stillimage" ? 0x00000001:
      props->tune == "fastdecode" ? 0x00000002:
      props->tune == "zerolatency" ? 0x00000004:
      0x00000004), // zerolatency
    "speed-preset", (
      props->speed_preset == "None" ? 0:
      props->speed_preset == "ultrafast" ? 1:
      props->speed_preset == "superfast" ? 2:
      props->speed_preset == "veryfast" ? 3:
      props->speed_preset == "faster" ? 4:
      props->speed_preset == "fast" ? 5:
      props->speed_preset == "medium" ? 6:
      props->speed_preset == "slow" ? 7:
      props->speed_preset == "slower" ? 8:
      props->speed_preset == "veryslow" ? 9:
      props->speed_preset == "placebo" ? 10:
      1), // ultrafast 
    "me", (
      props->me == "dia" ? 0:
      props->me == "hex" ? 1:
      props->me == "umh" ? 2:
      props->me == "esa" ? 3:
      props->me == "tesa" ? 4:
      0), // dia, faster
    "subme", props->subme, // Subpixel motion blur
    "threads", props->threads, // 1 is best for cpu and compression ratio
    "bitrate", props->bitrate,
    "noise-reduction", props->noise_reduction,
    "key-int-max", props->gop*props->framerate, // Largest GOP
    "vbv-buf-capacity", props->gop*1000,        // Buffer size for GOP
    "b-adapt", false, // Do not allow b frames
    "sliced-threads", false, // Do not sacrifice cpu usage for lower latency
    NULL);

  GstStructure *meta = gst_structure_new("meta", "serial", G_TYPE_STRING, props->serial.c_str(), NULL); 
  GstCaps *webrtc_caps = gst_caps_from_string(props->video_caps.c_str());
  g_object_set(webrtc,
      "do-fec", props->do_fec,
      "do-retransmission", props->do_retransmission,
      "congestion-control", (
        props->congestion_control == "disabled" ? 0 :
        props->congestion_control == "homegrown" ? 1 :
        props->congestion_control == "gcc" ? 2 :
        2),
      "meta", meta,
      "video-caps", webrtc_caps,
      NULL);
  gst_caps_unref(webrtc_caps);
  gst_structure_free(meta);

  gst_bin_add_many(GST_BIN(gst_pipeline), source, filter, encode, webrtc, NULL);

  bool ret = true;

  ret = gst_element_link(source, filter) ? ret : false;

  if (props->mime == "image/jpeg") {
    // Convert to hardware decoding if possible
    GstElement* decode = gst_element_factory_make(props->decoder.c_str(), "decoder");
    gst_bin_add(GST_BIN(gst_pipeline), decode);
    ret = gst_element_link(filter, decode) ? ret : false;
    ret = gst_element_link(decode, encode) ? ret : false;
  } else {
    GstElement* convert = gst_element_factory_make("videoconvert", "converter");
    gst_bin_add(GST_BIN(gst_pipeline), convert);
    ret = gst_element_link(filter, convert) ? ret : false;
    ret = gst_element_link(convert, encode) ? ret : false;
  }
  ret = gst_element_link(encode, webrtc) ? ret : false;
  if (!ret) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not link elements of pipeline for %s", props->serial.c_str());
      return nullptr;
  }
  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for h264software pipeline or sets defaults
*/

h264softwarePipelineProperties* get_h264software_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  h264softwarePipelineProperties* props = new h264softwarePipelineProperties;

  std::map<std::string, rclcpp::Parameter> serial_params;

  // Read yaml file
  YAML::Node config = YAML::LoadFile("/home/nova/nova/src/ros/cameras/cameras2++/params/streamer.yaml");
  YAML::Node param = config["camera_streamer"]["ros__parameters"][std::string(PIPELINE_PREFIX)][camera->serial];
  YAML::Node profile = config["camera_streamer"]["ros__parameters"]["profiles"][param["profile"].as<std::string>("default")];

  RCLCPP_INFO(streamer_node->get_logger(), "Getting props for %s", camera->serial.c_str());
  props->serial = camera->serial;
  props->node = camera->node;

  // override any defaults with params
  std::string camera_prefix = std::string(PIPELINE_PREFIX) + "." + camera->serial;
  props->device = param["device"] ? param["device"].as<std::string>() : profile["device"].as<std::string>(props->node);
  props->width = param["width"] ? param["width"].as<int>() : profile["width"].as<int>(1280);
  props->height = param["height"] ? param["height"].as<int>() : profile["height"].as<int>(720);
  props->framerate = param["framerate"] ? param["framerate"].as<int>() : profile["framerate"].as<int>(30);
  props->brightness = param["brightness"] ? param["brightness"].as<int>() : profile["brightness"].as<int>(0);
  props->contrast = param["contrast"] ? param["contrast"].as<int>() : profile["contrast"].as<int>(0);
  props->mime = param["mime"] ? param["mime"].as<std::string>() : profile["mime"].as<std::string>("image/jpeg");
  props->congestion_control = param["congestion_control"] ? param["congestion_control"].as<std::string>() : profile["congestion_control"].as<std::string>("gcc");
  props->do_fec = param["do_fec"] ? param["do_fec"].as<bool>() : profile["do_fec"].as<bool>(false);
  props->do_retransmission = param["do_retransmission"] ? param["do_retransmission"].as<bool>() : profile["do_retransmission"].as<bool>(false);
  props->video_caps = param["video_caps"] ? param["video_caps"].as<std::string>() : profile["video_caps"].as<std::string>("video/x-h264,profile=constrained-baseline");
  props->bitrate = param["bitrate"] ? param["bitrate"].as<int>() : profile["bitrate"].as<int>(8192);
  props->tune = param["tune"] ? param["tune"].as<std::string>() : profile["tune"].as<std::string>("zerolatency");
  props->speed_preset = param["speed_preset"] ? param["speed_preset"].as<std::string>() : profile["speed_preset"].as<std::string>("ultrafast");
  props->me = param["me"] ? param["me"].as<std::string>() : profile["me"].as<std::string>("dia");
  props->subme = param["subme"] ? param["subme"].as<int>() : profile["subme"].as<int>(1);
  props->noise_reduction = param["noise_reduction"] ? param["noise_reduction"].as<int>() : profile["noise_reduction"].as<int>(256);
  props->threads = param["threads"] ? param["threads"].as<int>() : profile["threads"].as<int>(1);
  props->gop = param["gop"] ? param["gop"].as<int>() : profile["gop"].as<int>(1); // Distance between frames, in seconds. Max 10
  props->decoder = param["decoder"] ? param["decoder"].as<std::string>() : profile["decoder"].as<std::string>(is_plugin_available("nvjpegdec") ? "nvjpegdec" : "jpegdec");
  props->io_mode = param["io_mode"] ? param["io_mode"].as<std::string>() : profile["io_mode"].as<std::string>("mmap");

  return props;
}

