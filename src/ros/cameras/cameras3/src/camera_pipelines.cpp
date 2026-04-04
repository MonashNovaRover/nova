#include <optional>
#include <string>

#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"

#include <camera_msgs/msg/camera.hpp>

#include "cameras/pipeline.hpp"

bool is_plugin_available(const std::string& plugin_name) {
  GstElementFactory* factory = gst_element_factory_find(plugin_name.c_str());
  if (factory != nullptr) {
      gst_object_unref(factory);
      return true;
  } 
  return false;
}

int crop43(const int width, const int height) {
  return (width-(height*4/3))/2;
}

bool link_elements(rclcpp::Node* streamer_node, GstElement* first_element, GstElement* second_element, const std::string serial) {
   if (!gst_element_link(first_element, second_element)) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not link %s to %s for %s", gst_object_get_name(GST_OBJECT(first_element)), gst_object_get_name(GST_OBJECT(second_element)), serial.c_str());
      return false;
   }
   return true;
}

std::string set_property(rclcpp::Node* streamer_node, const std::string serial, const std::string profile, const std::string original_serial, const std::string element, std::string value){
    // Get property
    streamer_node->get_parameter_or<std::string>((std::string(DEFAULT_PREFIX) + "." + original_serial + "." + element).c_str(), value, value);
    if (profile != "NULL")
      streamer_node->get_parameter_or<std::string>((std::string(PROFILE_PREFIX) + "." + profile + "." + element).c_str(), value, value);
    streamer_node->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + serial + "." + element).c_str(), value, value);
    return value;
}

int set_property(rclcpp::Node* streamer_node, const std::string serial, const std::string profile, const std::string original_serial, const std::string element, int value){
    // Get property
    streamer_node->get_parameter_or((std::string(DEFAULT_PREFIX) + "." + original_serial + "." + element).c_str(), value, value);
    if (profile != "NULL")
      streamer_node->get_parameter_or((std::string(PROFILE_PREFIX) + "." + profile + "." + element).c_str(), value, value);
    streamer_node->get_parameter_or((std::string(PIPELINE_PREFIX) + "." + serial + "." + element).c_str(), value, value);
    return value;
}

bool set_property(rclcpp::Node* streamer_node, const std::string serial, const std::string profile, const std::string original_serial, const std::string element, bool value){
    // Get property
    streamer_node->get_parameter_or((std::string(DEFAULT_PREFIX) + "." + original_serial + "." + element).c_str(), value, value);
    if (profile != "NULL")
      streamer_node->get_parameter_or((std::string(PROFILE_PREFIX) + "." + profile + "." + element).c_str(), value, value);
    streamer_node->get_parameter_or((std::string(PIPELINE_PREFIX) + "." + serial + "." + element).c_str(), value, value);
    return value;
}

void set_source(GstElement* source, auto props) {
    g_object_set(source,
      "device", props->device.c_str(),
      "io-mode", (
          props->io_mode == "rw" ? 1 :
          props->io_mode == "mmap" ? 2 :
          props->io_mode == "userptr" ? 3 :
          props->io_mode == "dmabuf" ? 4 :
          props->io_mode == "dmabuf-import" ? 5 :
          0),
      NULL);
}

void set_srcfilter(GstElement* srcfilter, auto props) {
    GstCaps *caps = gst_caps_new_simple(
        props->mime.c_str(),
        "width", G_TYPE_INT, props->width,
        "height", G_TYPE_INT, props->height,
        "framerate", GST_TYPE_FRACTION, props->framerate, 1,
        "brightness", G_TYPE_INT, props->brightness,
        "contrast", G_TYPE_INT,  props->contrast,
        NULL);
    g_object_set(srcfilter, "caps", caps, NULL);
    gst_caps_unref(caps);
}

void set_scalefilter(GstElement* scalefilter, auto props) {
    GstCaps *caps = gst_caps_new_simple(
        "video/x-raw",
        "width", G_TYPE_INT, props->width/props->downscale,
        "height", G_TYPE_INT, props->height/props->downscale,
        "framerate", GST_TYPE_FRACTION, props->framerate, 1,
        "brightness", G_TYPE_INT, props->brightness,
        "contrast", G_TYPE_INT,  props->contrast,
        NULL);
    g_object_set(scalefilter, "caps", caps, NULL);
    gst_caps_unref(caps);
}

void set_convert(GstElement* convert, auto props) {
  g_object_set(convert,
      "chroma-resampler", (
          props->chroma_resampler == "nearest" ? 0 :
          props->chroma_resampler == "linear" ? 1 :
          props->chroma_resampler == "cubic" ? 2 :
          props->chroma_resampler == "sinc" ? 3 : 
          props->chroma_resampler == "lanczos" ? 4 :
          0),
      "dither", (
          props->dither == "none" ? 0 :
          props->dither == "verterr" ? 1 :
          props->dither == "floyd-steinberg" ? 2 :
          props->dither == "sierra-lite" ? 3 : 
          props->dither == "bayer" ? 4 :
          4),
      "method", (
          props->method == "nearest-neighbour" ? 0 :
          props->method == "bilinear" ? 1 :
          props->method == "4-tap" ? 2 :
          props->method == "lanczos" ? 3 : 
          props->method == "bilinear2" ? 4 :
          props->method == "sinc" ? 5 :
          props->method == "hermite" ? 6 :
          props->method == "spline" ? 7 :
          props->method == "catrom" ? 8 : 
          props->method == "mitchell" ? 9 :
          0),
      NULL);
}

void set_crop43(GstElement* cropper, auto props, const int crop_width) {
    if (props->crop43) {
        g_object_set(cropper,
            "left", crop_width,
            "right", crop_width,
          NULL);
    }
}

void set_webrtc(GstElement* webrtc, auto props){
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
}

void set_payload(GstElement* payload, auto props) {
    if (props->payload_quirk) {
        // Apply patch for gc2093
        g_object_set(payload,
            "aggregate-mode", 1,
            "config-interval", -1,
            NULL);
    }
}

void set_x264(GstElement* encode, h264softwarePipelineProperties* props) {
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
}

void set_vpXenc(GstElement* encode, vpXsoftwarePipelineProperties* props) {
    g_object_set(encode,
        "deadline", props->deadline, // 1 for lowest latency
        "cpu-used", props->cpu_used, // Fastest -16, 16 Slowest 
        "end-usage", (
            props->end_usage == "vbr" ? 0:
            props->end_usage == "cbr" ? 1:
            props->end_usage == "cq" ? 2:
            1), // mode, constant bitrate best
        "threads", props->threads, // 1 is best for cpu and compression ratio
        "target-bitrate", props->bitrate*1000,
        "keyframe-max-dist", props->gop*props->framerate, // Largest GOP
        "buffer-optimal-size", props->gop*1000,        // Buffer size for GOP
        NULL);
}


void set_h264parse(GstElement* parse, const int interval = -1) {
    g_object_set(parse,
    "config-interval",
    interval,
    NULL);
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
  GstElement* decode = gst_element_factory_make("decodebin3", "decoder");
  GstElement* convert = gst_element_factory_make("videoconvertscale", "converter");
  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* clock = props->show_clock ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* cropper = props->crop43 ? gst_element_factory_make("videocrop", "video-cropper") : nullptr;
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");

  if (!gst_pipeline || !source || !srcfilter || !decode || !convert || !scalefilter || (props->show_clock && !clock) || (props->crop43 && !cropper) || !webrtc 
      ) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }
  RCLCPP_INFO(streamer_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate);

  // 2. Set element properties
  set_source(source, props);
  set_srcfilter(srcfilter, props);
  set_convert(convert, props);
  set_scalefilter(scalefilter, props);
  set_crop43(cropper, props, crop_width);
  set_webrtc(webrtc, props);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, srcfilter, decode, convert, scalefilter, webrtc, NULL);
  if (props->crop43) gst_bin_add(GST_BIN(gst_pipeline), cropper);
  if (props->show_clock) gst_bin_add(GST_BIN(gst_pipeline), clock);

  // 4. Link elements
  if (!link_elements(streamer_node, source, srcfilter, props->serial)) return nullptr;
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

v4l2webrtcPipelineProperties* get_v4l2webrtc_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  /*
    Pulls ros2 parameters for a given camera and returns a properties struct for the v4l2webrtc pipeline creation function.
    @param streamer_node pointer to the ros2 streamer node
    @param camera pointer to the camera message containing at least the serial and node for the camera
    @return pointer to a v4l2webrtcPipelineProperties struct containing the properties for the pipeline
  */

  // 0. Initialize constants
  v4l2webrtcPipelineProperties* props = new v4l2webrtcPipelineProperties;
  RCLCPP_INFO(streamer_node->get_logger(), "Getting props for %s", camera->serial.c_str());
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // Get profile
  std::string profile;
  streamer_node->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + camera->serial + ".profile").c_str(), profile, "NULL"); 

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = set_property(streamer_node, camera->serial, profile, camera->original_serial, "device", props->node);
  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera->serial, profile, camera->original_serial, "io_mode", default_string);

  // filter
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera->serial, profile, camera->original_serial, "mime", default_string);

  props->brightness = set_property(streamer_node, camera->serial, profile, camera->original_serial, "brightness", 0);
  props->contrast = set_property(streamer_node, camera->serial, profile, camera->original_serial, "contrast", 0);
  props->framerate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate", 30);
  props->height = set_property(streamer_node, camera->serial, profile, camera->original_serial, "height", 720);
  props->width = set_property(streamer_node, camera->serial, profile, camera->original_serial, "width", 1280);

  // convert
  default_string = "nearest";
  props->chroma_resampler = set_property(streamer_node, camera->serial, profile, camera->original_serial, "chroma_resampler", default_string);
  default_string = "bayer";
  props->dither = set_property(streamer_node, camera->serial, profile, camera->original_serial, "dither", default_string);
  default_string = "nearest-neighbour";
  props->method = set_property(streamer_node, camera->serial, profile, camera->original_serial, "method", default_string);

  // scale
  props->downscale = set_property(streamer_node, camera->serial, profile, camera->original_serial, "downscale", 1);

  // cropper
  props->crop43 = set_property(streamer_node, camera->serial, profile, camera->original_serial, "crop43", false);

  // clock
  props->show_clock = set_property(streamer_node, camera->serial, profile, camera->original_serial, "show_clock", false);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera->serial, profile, camera->original_serial, "congestion_control", default_string);
  default_string = "video/x-h264,profile=constrained-baseline"; 
  props->video_caps = set_property(streamer_node, camera->serial, profile, camera->original_serial, "video_caps", default_string);

  props->do_fec = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_retransmission", false);

  return props;
}

/*
 * V4l camera (h264) to webrtc pipeline (direct)
 * Enforces alignment from h264 v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-h264
 */
GstElement* h264passthrough_pipeline(rclcpp::Node* streamer_node, h264passthroughPipelineProperties* props)
{
  // 1. Create the elements
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");
  GstElement* parse = gst_element_factory_make("h264parse", "parser");
  GstElement* payload = (props->payload_quirk) ? gst_element_factory_make("rtph264pay", "payloader") : nullptr;
  GstElement* depayload = (props->payload_quirk) ? gst_element_factory_make("rtph264depay", "depayloader") : nullptr;

  if (!gst_pipeline || !source || !srcfilter || !parse || !webrtc || (props->payload_quirk && !payload) || (props->payload_quirk && !depayload)) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }
  RCLCPP_INFO(streamer_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate);

  // 2. Set element properties
  set_source(source, props);
  set_srcfilter(srcfilter, props);
  set_h264parse(parse);
  set_webrtc(webrtc, props);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, srcfilter, parse, webrtc, NULL);
  if (props->payload_quirk) gst_bin_add_many(GST_BIN(gst_pipeline), payload, depayload, NULL);

  // 4. Link elements
  if (!link_elements(streamer_node, source, srcfilter, props->serial)) return nullptr;

  if (props->payload_quirk) {
    if (!link_elements(streamer_node, srcfilter, payload, props->serial)) return nullptr;
    if (!link_elements(streamer_node, payload, depayload, props->serial)) return nullptr;
    if (!link_elements(streamer_node, depayload, parse, props->serial)) return nullptr;
  } else {
    if (!link_elements(streamer_node, srcfilter, parse, props->serial)) return nullptr;
  }
  if (!link_elements(streamer_node, parse, webrtc, props->serial)) return nullptr;

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for h264passthrough pipeline or sets defaults
*/

h264passthroughPipelineProperties* get_h264passthrough_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  // 0. Initialize constants
  h264passthroughPipelineProperties* props = new h264passthroughPipelineProperties;
  RCLCPP_INFO(streamer_node->get_logger(), "Getting props for %s", camera->serial.c_str());
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // Get profile
  std::string profile;
  streamer_node->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + camera->serial + ".profile").c_str(), profile, "NULL"); 

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = set_property(streamer_node, camera->serial, profile, camera->original_serial, "device", props->node);
  props->io_mode = 4; // dmabuf

  // filter
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera->serial, profile, camera->original_serial, "mime", default_string);

  props->brightness = set_property(streamer_node, camera->serial, profile, camera->original_serial, "brightness", 0);
  props->contrast = set_property(streamer_node, camera->serial, profile, camera->original_serial, "contrast", 0);
  props->framerate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate", 30);
  props->height = set_property(streamer_node, camera->serial, profile, camera->original_serial, "height", 720);
  props->width = set_property(streamer_node, camera->serial, profile, camera->original_serial, "width", 1280);

  // payloader
  props->payload_quirk = set_property(streamer_node, camera->serial, profile, camera->original_serial, "payload_quirk", false);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera->serial, profile, camera->original_serial, "congestion_control", default_string);
  props->video_caps = "video/x-h264";

  props->do_fec = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_retransmission", false);

  return props;
}

/*
 * V4l camera (any) decoded then encoded into x264enc
 * Enforces alignment from h264 v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-h264
 */

GstElement* h264software_pipeline(rclcpp::Node* streamer_node, h264softwarePipelineProperties* props)
{
  // 0. Initialize constants
  // Disable crop43 if it is already 4:3 or jpeg
  const int crop_width = crop43(props->width, props->height);
  if (crop_width == 0) {
      props->crop43 = false;
  }

  // 1. Create the elements
  GstElement* gst_pipeline = gst_pipeline_new(props->serial.c_str());
  GstElement* source = gst_element_factory_make("v4l2src", "video-source");
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* convert = gst_element_factory_make("videoconvertscale", "converter");
  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* encode = gst_element_factory_make("x264enc", "encoder");
  GstElement* parse = gst_element_factory_make("h264parse", "parser");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");
  GstElement* clock = (props->show_clock) ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* cropper = (props->crop43) ? gst_element_factory_make("videocrop", "video-cropper") : nullptr;
  GstElement* decode = (props->mime == "image/jpeg") ? gst_element_factory_make(props->decoder.c_str(), "decoder") : nullptr;


  if (!gst_pipeline || !source || !srcfilter || !convert || !scalefilter || !encode || (props->show_clock && !clock) || (props->crop43 && !cropper) || (props->mime == "video/x-raw" && !convert) || (props->mime == "image/jpeg" && !decode) || !parse || !webrtc) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }
  RCLCPP_INFO(streamer_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate);
  
  // 2. Set element properties
  set_source(source, props);
  set_srcfilter(srcfilter, props);
  set_convert(convert, props);
  set_scalefilter(scalefilter, props);
  set_crop43(cropper, props, crop_width);
  set_x264(encode, props);
  set_h264parse(parse, props->gop);
  set_webrtc(webrtc, props);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, srcfilter, convert, scalefilter, encode, parse, webrtc, NULL);
  if (props->crop43) gst_bin_add(GST_BIN(gst_pipeline), cropper);
  if (props->show_clock) gst_bin_add(GST_BIN(gst_pipeline), clock);
  if (props->mime == "video/x-raw") gst_bin_add(GST_BIN(gst_pipeline), convert);
  if (props->mime == "image/jpeg") gst_bin_add(GST_BIN(gst_pipeline), decode);

  // 4. Link elements
  if (!link_elements(streamer_node, source, srcfilter, props->serial)) return nullptr;

  // Convert to raw
  if (props->mime == "image/jpeg") {
      if (!link_elements(streamer_node, srcfilter, decode, props->serial)) return nullptr;
      if (!link_elements(streamer_node, decode, convert, props->serial)) return nullptr;
  } else {
      if (!link_elements(streamer_node, srcfilter, convert, props->serial)) return nullptr;
  }
  
  if (!link_elements(streamer_node, convert, scalefilter, props->serial)) return nullptr;

  // Enable crop and/or clock
  if (props->crop43 && props->show_clock) {
      if (!link_elements(streamer_node, scalefilter, cropper, props->serial)) return nullptr;
      if (!link_elements(streamer_node, cropper, clock, props->serial)) return nullptr;
      if (!link_elements(streamer_node, clock, encode, props->serial)) return nullptr;
  } else if (props->crop43) {
      if (!link_elements(streamer_node, scalefilter, cropper, props->serial)) return nullptr;
      if (!link_elements(streamer_node, cropper, encode, props->serial)) return nullptr;
  } else if (props->show_clock) {
      if (!link_elements(streamer_node, scalefilter, clock, props->serial)) return nullptr;
      if (!link_elements(streamer_node, clock, encode, props->serial)) return nullptr;
  } else {
      if (!link_elements(streamer_node, scalefilter, encode, props->serial)) return nullptr;
  }

  if (!link_elements(streamer_node, encode, parse, props->serial)) return nullptr;
  if (!link_elements(streamer_node, parse, webrtc, props->serial)) return nullptr;

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for h264software pipeline or sets defaults
*/

h264softwarePipelineProperties* get_h264software_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  // 0. Initialize constants
  h264softwarePipelineProperties* props = new h264softwarePipelineProperties;
  RCLCPP_INFO(streamer_node->get_logger(), "Getting props for %s", camera->serial.c_str());
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // Get profile
  std::string profile;
  streamer_node->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + camera->serial + ".profile").c_str(), profile, "NULL"); 

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = set_property(streamer_node, camera->serial, profile, camera->original_serial, "device", props->node);
  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera->serial, profile, camera->original_serial, "io_mode", "mmap");

  // filter
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera->serial, profile, camera->original_serial, "mime", default_string);

  props->brightness = set_property(streamer_node, camera->serial, profile, camera->original_serial, "brightness", 0);
  props->contrast = set_property(streamer_node, camera->serial, profile, camera->original_serial, "contrast", 0);
  props->framerate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate", 30);
  props->height = set_property(streamer_node, camera->serial, profile, camera->original_serial, "height", 720);
  props->width = set_property(streamer_node, camera->serial, profile, camera->original_serial, "width", 1280);

  // decoder
  default_string = is_plugin_available("nvjpegdec") ? "nvjpegdec" : "jpegdec";
  props->decoder = set_property(streamer_node, camera->serial, profile, camera->original_serial, "decoder", default_string);

  // convert
  default_string = "nearest";
  props->chroma_resampler = set_property(streamer_node, camera->serial, profile, camera->original_serial, "chroma_resampler", default_string);
  default_string = "bayer";
  props->dither = set_property(streamer_node, camera->serial, profile, camera->original_serial, "dither", default_string);
  default_string = "nearest-neighbour";
  props->method = set_property(streamer_node, camera->serial, profile, camera->original_serial, "method", default_string);

  // scale
  props->downscale = set_property(streamer_node, camera->serial, profile, camera->original_serial, "downscale", 1);

  // cropper
  props->crop43 = set_property(streamer_node, camera->serial, profile, camera->original_serial, "crop43", false);

  // clock
  props->show_clock = set_property(streamer_node, camera->serial, profile, camera->original_serial, "show_clock", false);

  // encode
  default_string = "dia";
  props->me = set_property(streamer_node, camera->serial, profile, camera->original_serial, "me", default_string);
  default_string = "ultrafast";
  props->speed_preset = set_property(streamer_node, camera->serial, profile, camera->original_serial, "speed_preset", default_string);
  default_string = "zerolatency";
  props->tune = set_property(streamer_node, camera->serial, profile, camera->original_serial, "tune", default_string);

  props->bitrate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "bitrate", 4096);
  props->gop = set_property(streamer_node, camera->serial, profile, camera->original_serial, "gop", 1);
  props->noise_reduction = set_property(streamer_node, camera->serial, profile, camera->original_serial, "noise_reduction", 256);
  props->subme = set_property(streamer_node, camera->serial, profile, camera->original_serial, "subme", 1);
  props->threads = set_property(streamer_node, camera->serial, profile, camera->original_serial, "threads", 1);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera->serial, profile, camera->original_serial, "congestion_control", default_string);
  default_string = "video/x-h264,profile=constrained-baseline";
  props->video_caps = set_property(streamer_node, camera->serial, profile, camera->original_serial, "video_caps", default_string);

  props->do_fec = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_retransmission", false);

  return props;
}

/*
 * V4l camera (any) decoded then encoded into vpXenc
 * Enforces alignment from vpX v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-h264
 */

GstElement* vpXsoftware_pipeline(rclcpp::Node* streamer_node, vpXsoftwarePipelineProperties* props)
{
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
  GstElement* convert = gst_element_factory_make("videoconvertscale", "converter");
  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* encode = (props->video_caps == "video/x-vp9") ? gst_element_factory_make("vp9enc", "encoder") : gst_element_factory_make("vp8enc", "encoder");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");
  GstElement* clock = (props->show_clock) ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* cropper = (props->crop43) ? gst_element_factory_make("videocrop", "video-cropper") : nullptr;
  GstElement* decode = (props->mime == "image/jpeg") ? gst_element_factory_make(props->decoder.c_str(), "decoder") : nullptr;


  if (!gst_pipeline || !source || !srcfilter || !convert || !scalefilter || !encode || (props->show_clock && !clock) || (props->crop43 && !cropper) || (props->mime == "video/x-raw" && !convert) || (props->mime == "image/jpeg" && !decode) || !webrtc) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }
  RCLCPP_INFO(streamer_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate);
  
  // 2. Set element properties
  set_source(source, props);
  set_srcfilter(srcfilter, props);
  set_convert(convert, props);
  set_scalefilter(scalefilter, props);
  set_crop43(cropper, props, crop_width);
  set_vpXenc(encode, props);
  set_webrtc(webrtc, props);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, srcfilter, convert, scalefilter, encode, webrtc, NULL);
  if (props->crop43) gst_bin_add(GST_BIN(gst_pipeline), cropper);
  if (props->show_clock) gst_bin_add(GST_BIN(gst_pipeline), clock);
  if (props->mime == "video/x-raw") gst_bin_add(GST_BIN(gst_pipeline), convert);
  if (props->mime == "image/jpeg") gst_bin_add(GST_BIN(gst_pipeline), decode);

  // 4. Link elements
  if (!link_elements(streamer_node, source, srcfilter, props->serial)) return nullptr;

  // Convert to raw
  if (props->mime == "image/jpeg") {
      if (!link_elements(streamer_node, srcfilter, decode, props->serial)) return nullptr;
      if (!link_elements(streamer_node, decode, convert, props->serial)) return nullptr;
  } else {
      if (!link_elements(streamer_node, srcfilter, convert, props->serial)) return nullptr;
  }
  
  if (!link_elements(streamer_node, convert, scalefilter, props->serial)) return nullptr;

  // Enable crop and/or clock
  if (props->crop43 && props->show_clock) {
      if (!link_elements(streamer_node, scalefilter, cropper, props->serial)) return nullptr;
      if (!link_elements(streamer_node, cropper, clock, props->serial)) return nullptr;
      if (!link_elements(streamer_node, clock, encode, props->serial)) return nullptr;
  } else if (props->crop43) {
      if (!link_elements(streamer_node, scalefilter, cropper, props->serial)) return nullptr;
      if (!link_elements(streamer_node, cropper, encode, props->serial)) return nullptr;
  } else if (props->show_clock) {
      if (!link_elements(streamer_node, scalefilter, clock, props->serial)) return nullptr;
      if (!link_elements(streamer_node, clock, encode, props->serial)) return nullptr;
  } else {
      if (!link_elements(streamer_node, scalefilter, encode, props->serial)) return nullptr;
  }

  if (!link_elements(streamer_node, encode, webrtc, props->serial)) return nullptr;

  return gst_pipeline;
}


/*
 * Retrieve ros2 parameters for vpXsoftware pipeline or sets defaults
*/

vpXsoftwarePipelineProperties* get_vpXsoftware_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  // 0. Initialize constants
  vpXsoftwarePipelineProperties* props = new vpXsoftwarePipelineProperties;
  RCLCPP_INFO(streamer_node->get_logger(), "Getting props for %s", camera->serial.c_str());
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // Get profile
  std::string profile;
  streamer_node->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + camera->serial + ".profile").c_str(), profile, "NULL"); 

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = set_property(streamer_node, camera->serial, profile, camera->original_serial, "device", props->node);
  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera->serial, profile, camera->original_serial, "io_mode", "mmap");

  // filter
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera->serial, profile, camera->original_serial, "mime", default_string);

  props->brightness = set_property(streamer_node, camera->serial, profile, camera->original_serial, "brightness", 0);
  props->contrast = set_property(streamer_node, camera->serial, profile, camera->original_serial, "contrast", 0);
  props->framerate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate", 30);
  props->height = set_property(streamer_node, camera->serial, profile, camera->original_serial, "height", 720);
  props->width = set_property(streamer_node, camera->serial, profile, camera->original_serial, "width", 1280);

  // decoder
  default_string = is_plugin_available("nvjpegdec") ? "nvjpegdec" : "jpegdec";
  props->decoder = set_property(streamer_node, camera->serial, profile, camera->original_serial, "decoder", default_string);

  // convert
  default_string = "nearest";
  props->chroma_resampler = set_property(streamer_node, camera->serial, profile, camera->original_serial, "chroma_resampler", default_string);
  default_string = "bayer";
  props->dither = set_property(streamer_node, camera->serial, profile, camera->original_serial, "dither", default_string);
  default_string = "nearest-neighbour";
  props->method = set_property(streamer_node, camera->serial, profile, camera->original_serial, "method", default_string);

  // scale
  props->downscale = set_property(streamer_node, camera->serial, profile, camera->original_serial, "downscale", 1);

  // cropper
  props->crop43 = set_property(streamer_node, camera->serial, profile, camera->original_serial, "crop43", false);

  // clock
  props->show_clock = set_property(streamer_node, camera->serial, profile, camera->original_serial, "show_clock", false);

  // encode
  default_string = "cbr";
  props->end_usage = set_property(streamer_node, camera->serial, profile, camera->original_serial, "end_usage", default_string);

  props->bitrate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "bitrate", 4096);
  props->cpu_used = set_property(streamer_node, camera->serial, profile, camera->original_serial, "cpu_used", 16);
  props->deadline = set_property(streamer_node, camera->serial, profile, camera->original_serial, "deadline", 1);
  props->gop = set_property(streamer_node, camera->serial, profile, camera->original_serial, "gop", 1);
  props->threads = set_property(streamer_node, camera->serial, profile, camera->original_serial, "threads", 1);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera->serial, profile, camera->original_serial, "congestion_control", default_string);
  default_string = "video/x-vp8";
  props->video_caps = set_property(streamer_node, camera->serial, profile, camera->original_serial, "video_caps", default_string);

  props->do_fec = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_retransmission", false);

  return props;
}

