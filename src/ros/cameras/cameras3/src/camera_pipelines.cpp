#include <string>

#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"
#include <camera_msgs/msg/camera.hpp>

#include "cameras/pipeline.hpp"



bool verify_resolution(auto props) {
  GstDeviceMonitor *monitor = gst_device_monitor_new();
  gst_device_monitor_add_filter(monitor, "Video/Source", NULL);

  GList *devices = gst_device_monitor_get_devices(monitor);
  for (GList *l = devices; l != NULL; l = l->next) {
      GstDevice *device = (GstDevice *)l->data;
      GstStructure *device_props = gst_device_get_properties(device);
      const gchar *path = gst_structure_get_string(device_props, "device.path");
      int valid_width = 1280, valid_height = 720, framerate_n = 30, framerate_d = 1;
      std::string valid_mime;

      if (std::string(path) == props->device) {
          GstCaps* caps = gst_device_get_caps(device);
          for (guint i = 0; i < gst_caps_get_size(caps); i++) {
              const GstStructure* str = gst_caps_get_structure(caps, i);
              valid_mime = std::string(gst_structure_get_name(str));

              // Width
              const GValue* width_val = gst_structure_get_value(str, "width");
              bool width_ok = false;

              if (G_VALUE_HOLDS_INT(width_val)) {
                  width_ok = (valid_width == props->width);
                  valid_width = g_value_get_int(width_val);
              } else if (GST_VALUE_HOLDS_INT_RANGE(width_val)) {
                  width_ok = (props->width >= gst_value_get_int_range_min(width_val) &&
                              props->width <= gst_value_get_int_range_max(width_val));
                  valid_width = gst_value_get_int_range_min(width_val);
              } else if (GST_VALUE_HOLDS_LIST(width_val)) {
                  for (guint j = 0; j < gst_value_list_get_size(width_val); ++j) {
                      const GValue* v = gst_value_list_get_value(width_val, j);
                      if (g_value_get_int(v) == props->width) {
                          width_ok = true;
                          valid_width = g_value_get_int(v);
                          break;
                      }
                  }
              }

              // Height
              const GValue* height_val = gst_structure_get_value(str, "height");
              bool height_ok = false;

              if (G_VALUE_HOLDS_INT(height_val)) {
                  height_ok = (valid_height == props->height);
                  valid_height = g_value_get_int(height_val);
              } else if (GST_VALUE_HOLDS_INT_RANGE(height_val)) {
                  height_ok = (props->height >= gst_value_get_int_range_min(height_val) &&
                               props->height <= gst_value_get_int_range_max(height_val));
                  valid_height = gst_value_get_int_range_max(height_val);
              } else if (GST_VALUE_HOLDS_LIST(height_val)) {
                  for (guint j = 0; j < gst_value_list_get_size(height_val); ++j) {

                      const GValue* v = gst_value_list_get_value(height_val, j);
                      if (g_value_get_int(v) == props->height) {
                          height_ok = true;
                          valid_height = g_value_get_int(v);
                          break;
                      }
                  }
              }

              // Framerate
              gst_structure_get_fraction(str, "framerate", &framerate_n, &framerate_d);

              if ((valid_mime == props->mime) && width_ok && height_ok && (framerate_n == props->framerate)) {
                g_list_free_full(devices, gst_object_unref);
                gst_object_unref(monitor);
                return true;
              }
          }
          g_list_free_full(devices, gst_object_unref);
          gst_object_unref(monitor);

          props->mime = valid_mime;
          props->width = valid_width;
          props->height = valid_height;
          props->framerate = framerate_n;
          props->framerate_denominator = framerate_d;

          return false;
      }
  }
  g_list_free_full(devices, gst_object_unref);
  gst_object_unref(monitor);
  return true;
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

std::string set_property(rclcpp::Node* streamer_node, const std::string serial, const std::string profile, const std::string original_serial, const std::string element, const std::string default_value){
    // Get property
    std::string value;
    streamer_node->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + serial + "." + element).c_str(), value, default_value);
    if (value != default_value) return value;
    if (profile != "NULL") {
      streamer_node->get_parameter_or<std::string>((std::string(PROFILE_PREFIX) + "." + profile + "." + element).c_str(), value, default_value);
      if (value != default_value) return value;
    }
    return value;
    streamer_node->get_parameter_or<std::string>((std::string(DEFAULT_PREFIX) + "." + original_serial + "." + element).c_str(), value, default_value);
}

int set_property(rclcpp::Node* streamer_node, const std::string serial, const std::string profile, const std::string original_serial, const std::string element, const int default_value){
    // Get property
    int value;
    streamer_node->get_parameter_or((std::string(PIPELINE_PREFIX) + "." + serial + "." + element).c_str(), value, default_value);
    if (value != default_value) return value;
    if (profile != "NULL") {
      streamer_node->get_parameter_or((std::string(PROFILE_PREFIX) + "." + profile + "." + element).c_str(), value, default_value);
      if (value != default_value) return value;
    }
    streamer_node->get_parameter_or((std::string(DEFAULT_PREFIX) + "." + original_serial + "." + element).c_str(), value, default_value);
    return value;
}

bool set_property(rclcpp::Node* streamer_node, const std::string serial, const std::string profile, const std::string original_serial, const std::string element, const bool default_value){
    // Get property
    bool value;
    streamer_node->get_parameter_or((std::string(PIPELINE_PREFIX) + "." + serial + "." + element).c_str(), value, default_value);
    if (value != default_value) return value;
    if (profile != "NULL") {
      streamer_node->get_parameter_or((std::string(PROFILE_PREFIX) + "." + profile + "." + element).c_str(), value, default_value);
      if (value != default_value) return value;
    }
    streamer_node->get_parameter_or((std::string(DEFAULT_PREFIX) + "." + original_serial + "." + element).c_str(), value, default_value);
    return value;
}

void set_source(GstElement* source, const std::string device, const std::string io_mode) {
    g_object_set(source,
      "device", device.c_str(),
      "io-mode", (
          io_mode == "rw" ? 1 :
          io_mode == "mmap" ? 2 :
          io_mode == "userptr" ? 3 :
          io_mode == "dmabuf" ? 4 :
          io_mode == "dmabuf-import" ? 5 :
          0),
      NULL);
}

void set_srcfilter(GstElement* filter, const std::string mime, const int width, const int height, const int framerate, const int framerate_denominator, const int downrate, const int brightness, const int contrast) {
  GstCaps *caps = gst_caps_new_simple(
      mime.c_str(),
      "width", G_TYPE_INT, width,
      "height", G_TYPE_INT, height,
      "framerate", GST_TYPE_FRACTION, framerate, framerate_denominator*downrate,
      "brightness", G_TYPE_INT, brightness,
      "contrast", G_TYPE_INT,  contrast,
      NULL);
  g_object_set(filter, "caps", caps, NULL);
  gst_caps_unref(caps);
}

void set_scalefilter(GstElement* filter, const std::string format, const int width, const int height, const int framerate, const int framerate_denominator, const int downscale, const int downrate, const int brightness, const int contrast) {
  const std::string mime = "video/x-raw";
  GstCaps *caps = gst_caps_new_simple(
      mime.c_str(),
      "format", G_TYPE_STRING, format.c_str(),
      "width", G_TYPE_INT, width/downscale,
      "height", G_TYPE_INT, height/downscale,
      "framerate", GST_TYPE_FRACTION, framerate, framerate_denominator*downrate,
      "brightness", G_TYPE_INT, brightness,
      "contrast", G_TYPE_INT,  contrast,
      NULL);
  g_object_set(filter, "caps", caps, NULL);
  gst_caps_unref(caps);
}

void set_convert(GstElement* convert, const std::string chroma_resampler, const std::string dither, const std::string method) {
  g_object_set(convert,
      "chroma-resampler", (
          chroma_resampler == "nearest" ? 0 :
          chroma_resampler == "linear" ? 1 :
          chroma_resampler == "cubic" ? 2 :
          chroma_resampler == "sinc" ? 3 : 
          chroma_resampler == "lanczos" ? 4 :
          0),
      "dither", (
          dither == "none" ? 0 :
          dither == "verterr" ? 1 :
          dither == "floyd-steinberg" ? 2 :
          dither == "sierra-lite" ? 3 : 
          dither == "bayer" ? 4 :
          4),
      "method", (
          method == "nearest-neighbour" ? 0 :
          method == "bilinear" ? 1 :
          method == "4-tap" ? 2 :
          method == "lanczos" ? 3 : 
          method == "bilinear2" ? 4 :
          method == "sinc" ? 5 :
          method == "hermite" ? 6 :
          method == "spline" ? 7 :
          method == "catrom" ? 8 : 
          method == "mitchell" ? 9 :
          0),
      NULL);
}

void set_crop43(GstElement* cropper, const bool crop43, const int crop_width, const int downscale) {
    if (crop43) {
        g_object_set(cropper,
          "left", crop_width/downscale,
          "right", crop_width/downscale,
          NULL);
    }
}

void set_webrtc(GstElement* webrtc, const std::string serial, const std::string video_caps, const bool do_fec, const bool do_retransmission, const std::string congestion_control, const int bitrate){
    GstStructure *meta = gst_structure_new("meta", "serial", G_TYPE_STRING, serial.c_str(), NULL); 
    GstCaps *webrtc_caps = gst_caps_from_string(video_caps.c_str());
    g_object_set(webrtc,
        "do-fec", do_fec,
        "do-retransmission", do_retransmission,
        "congestion-control", (
            congestion_control == "disabled" ? 0 :
            congestion_control == "homegrown" ? 1 :
            congestion_control == "gcc" ? 2 :
            2),
        "max-bitrate", bitrate*1125,
        "meta", meta,
        "video-caps", webrtc_caps,
        NULL);
    gst_caps_unref(webrtc_caps);
    gst_structure_free(meta);
}

void set_payload(GstElement* payload, const bool payload_quirk) {
    if (payload_quirk) {
        // Apply patch for gc2093
        g_object_set(payload,
            "aggregate-mode", 1,
            "config-interval", -1,
            NULL);
    }
}

void set_x264(GstElement* encode, const std::string tune, const std::string speed_preset, const int threads, const int bitrate, const int noise_reduction, const int gop, const int framerate, const int framerate_denominator, const int downrate) {
    g_object_set(encode,
        "tune", ( 
            tune == "stillimage" ? 0x00000001:
            tune == "fastdecode" ? 0x00000002:
            tune == "zerolatency" ? 0x00000004:
            0x00000004), // zerolatency
        "speed-preset", (
            speed_preset == "None" ? 0:
            speed_preset == "ultrafast" ? 1:
            speed_preset == "superfast" ? 2:
            speed_preset == "veryfast" ? 3:
            speed_preset == "faster" ? 4:
            speed_preset == "fast" ? 5:
            speed_preset == "medium" ? 6:
            speed_preset == "slow" ? 7:
            speed_preset == "slower" ? 8:
            speed_preset == "veryslow" ? 9:
            speed_preset == "placebo" ? 10:
            1), // ultrafast 
        "threads", threads, // 1 is best for cpu and compression ratio
        "bitrate", bitrate,
        "noise-reduction", noise_reduction,
        "key-int-max", (int) gop * (int) ((float) framerate/ (float) framerate_denominator/ (float) downrate + 1.0), // Largest GOP
        "vbv-buf-capacity", gop*1000,        // Buffer size for GOP
        "b-adapt", false, // Do not allow b frames
        "sliced-threads", false, // Do not sacrifice cpu usage for lower latency
        NULL);
}

void set_vpXenc(GstElement* encode, const int deadline, const int cpu_used, const std::string end_usage, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate, const std::string video_caps, const std::string aq_mode) {
    g_object_set(encode,
        "deadline", deadline, // 1 for lowest latency
        "cpu-used", cpu_used, // Fastest -16, 16 Slowest 
        "end-usage", (
            end_usage == "vbr" ? 0:
            end_usage == "cbr" ? 1:
            end_usage == "cq" ? 2:
            1), // mode, constant bitrate best
        "threads", threads, // 1 is best for cpu and compression ratio
        "target-bitrate", bitrate*1000,
        "keyframe-max-dist", (int) gop * (int) ((float) framerate/ (float) framerate_denominator/ (float) downrate + 1.0), // Largest GOP
        "buffer-optimal-size", gop*1000,        // Buffer size for GOP
        "lag-in-frames", 0, // Do not lookahead
        "error-resilient", 1,
        NULL);
    
    if (video_caps == "video/x-vp9") {
        g_object_set(encode,
            "aq-mode", (
                aq_mode == "off" ? 0 :
                aq_mode == "variance" ? 1 :
                aq_mode == "complexity" ? 2 :
                aq_mode == "cyclic-refresh" ? 3 :
                aq_mode == "equator360" ? 4 :
                aq_mode == "perceptual" ? 5 :
                aq_mode == "psnr" ? 6 :
                aq_mode == "lookahead" ? 7 :
                5),
            "tile-columns", threads,
            "tile-rows", threads,
          NULL);
    }
}

void set_av1enc(GstElement* encode, const int cpu_used, const std::string end_usage, const std::string usage_profile, const int threads, const int bitrate, const int gop, const int framerate, const int framerate_denominator, const int downrate) {
    g_object_set(encode,
        "cpu-used", cpu_used, // Fastest 10, 1 Slowest 
        "end-usage", (
            end_usage == "vbr" ? 0:
            end_usage == "cbr" ? 1:
            end_usage == "cq" ? 2:
            1), // mode, constant bitrate best
        "usage-profile", (
            usage_profile == "good-quality" ? 0:
            usage_profile == "realtime" ? 1:
            usage_profile == "all-intra" ? 2:
            2), 
        "threads", threads, // 1 is best for cpu and compression ratio
        "target-bitrate", bitrate,
        "keyframe-max-dist", (int) gop * (int) ((float) framerate/ (float) framerate_denominator/ (float) downrate + 1.0), // Largest GOP
        "tile-columns", threads,
        "tile-rows", threads,
        NULL);
}

void set_h264parse(GstElement* parse, const int interval = -1) {
    g_object_set(parse,
        "config-interval", interval,
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
  GstElement* rate = (props->downrate > 1) ? gst_element_factory_make("videorate", "rater") : nullptr;
  GstElement* decode = gst_element_factory_make("decodebin3", "decoder");
  GstElement* convert = gst_element_factory_make("videoconvertscale", "converter");
  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* clock = props->show_clock ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* cropper = props->crop43 ? gst_element_factory_make("videocrop", "video-cropper") : nullptr;
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");


  if (!gst_pipeline || !source || (props->downrate > 1 && !rate) || !srcfilter || !decode || !convert || !scalefilter || (props->show_clock && !clock) || (props->crop43 && !cropper) || !webrtc 
      ) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }

  // Verify resolution
  if (verify_resolution(props)) {
      RCLCPP_INFO(streamer_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate/props->framerate_denominator);
  } else {
      RCLCPP_ERROR(streamer_node->get_logger(), "Wrong resolution! Fallback pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate/props->framerate_denominator);
  };

  // 2. Set element properties
  set_source(source, props->device, props->io_mode);
  set_srcfilter(srcfilter, props->mime, props->width, props->height, props->framerate, props->framerate_denominator, props->downrate, props->brightness, props->contrast);
  set_convert(convert, props->chroma_resampler, props->dither, props->method);
  set_scalefilter(scalefilter, props->format, props->width, props->height, props->framerate, props->framerate_denominator, props->downscale, props->downrate, props->brightness, props->contrast);
  set_crop43(cropper, props->crop43, crop_width, props->downscale);
  set_webrtc(webrtc, props->serial, props->video_caps, props->do_fec, props->do_retransmission, props->congestion_control, props->bitrate);

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
  RCLCPP_DEBUG(streamer_node->get_logger(), "Getting props for %s", camera->serial.c_str());
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // Get profile
  std::string profile = "NULL";
  streamer_node->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + camera->serial + ".profile").c_str(), profile, profile);
  streamer_node->get_parameter_or<std::string>((std::string(DEFAULT_PREFIX) + "." + camera->original_serial + ".profile").c_str(), profile, profile);

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = set_property(streamer_node, camera->serial, profile, camera->original_serial, "device", props->node);
  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera->serial, profile, camera->original_serial, "io_mode", default_string);

  // filter
  default_string = "I420";
  props->format = set_property(streamer_node, camera->serial, profile, camera->original_serial, "format", default_string);
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera->serial, profile, camera->original_serial, "mime", default_string);

  props->brightness = set_property(streamer_node, camera->serial, profile, camera->original_serial, "brightness", 0);
  props->contrast = set_property(streamer_node, camera->serial, profile, camera->original_serial, "contrast", 0);
  props->framerate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate", 30);
  props->framerate_denominator = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate_denominator", 1);
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

  // rate
  props->downrate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "downrate", 1);

  // cropper
  props->crop43 = set_property(streamer_node, camera->serial, profile, camera->original_serial, "crop43", false);

  // clock
  props->show_clock = set_property(streamer_node, camera->serial, profile, camera->original_serial, "show_clock", false);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera->serial, profile, camera->original_serial, "congestion_control", default_string);
  default_string = "video/x-h264,profile=constrained-baseline"; 
  props->video_caps = set_property(streamer_node, camera->serial, profile, camera->original_serial, "video_caps", default_string);

  props->bitrate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "bitrate", 4096);

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
  GstElement* parse = gst_element_factory_make("h264parse", "parser");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");
  GstElement* payload = (props->payload_quirk) ? gst_element_factory_make("rtph264pay", "payloader") : nullptr;
  GstElement* depayload = (props->payload_quirk) ? gst_element_factory_make("rtph264depay", "depayloader") : nullptr;

  if (!gst_pipeline || !source || !srcfilter || !parse || !webrtc || (props->payload_quirk && !payload) || (props->payload_quirk && !depayload)) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }

  // Verify resolution
  if (verify_resolution(props)) {
      RCLCPP_INFO(streamer_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate/props->framerate_denominator);
  } else {
      RCLCPP_ERROR(streamer_node->get_logger(), "Wrong resolution! Fallback pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate/props->framerate_denominator);
  };

  // 2. Set element properties
  set_source(source, props->device, props->io_mode);
  set_srcfilter(srcfilter, props->mime, props->width, props->height, props->framerate, props->framerate_denominator, props->downrate, props->brightness, props->contrast);
  set_payload(payload, props->payload_quirk);
  set_h264parse(parse);
  set_webrtc(webrtc, props->serial, props->video_caps, props->do_fec, props->do_retransmission, props->congestion_control, props->bitrate);

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
  RCLCPP_DEBUG(streamer_node->get_logger(), "Getting props for %s", camera->serial.c_str());
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // Get profile
  std::string profile = "NULL";
  streamer_node->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + camera->serial + ".profile").c_str(), profile, profile);
  streamer_node->get_parameter_or<std::string>((std::string(DEFAULT_PREFIX) + "." + camera->original_serial + ".profile").c_str(), profile, profile);

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = set_property(streamer_node, camera->serial, profile, camera->original_serial, "device", props->node);
  props->io_mode = 4; // dmabuf

  // rate
  props->downrate = 1; // Do not change framerate

  // filter
  props->mime = "video/x-h264";

  props->brightness = set_property(streamer_node, camera->serial, profile, camera->original_serial, "brightness", 0);
  props->contrast = set_property(streamer_node, camera->serial, profile, camera->original_serial, "contrast", 0);
  props->framerate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate", 30);
  props->framerate_denominator = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate_denominator", 1);
  props->height = set_property(streamer_node, camera->serial, profile, camera->original_serial, "height", 720);
  props->width = set_property(streamer_node, camera->serial, profile, camera->original_serial, "width", 1280);

  // payloader
  props->payload_quirk = set_property(streamer_node, camera->serial, profile, camera->original_serial, "payload_quirk", false);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera->serial, profile, camera->original_serial, "congestion_control", default_string);
  props->video_caps = "video/x-h264";

  props->bitrate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "bitrate", 4096);

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
  GstElement* rate = (props->downrate > 1) ? gst_element_factory_make("videorate", "rater") : nullptr;
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* decode = (props->mime == "image/jpeg") ? gst_element_factory_make(props->decoder.c_str(), "decoder") : nullptr;
  GstElement* convert = gst_element_factory_make("videoconvertscale", "converter");
  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* clock = (props->show_clock) ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* cropper = (props->crop43) ? gst_element_factory_make("videocrop", "video-cropper") : nullptr;
  GstElement* encode = gst_element_factory_make("x264enc", "encoder");
  GstElement* parse = gst_element_factory_make("h264parse", "parser");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");

  if (!gst_pipeline || !source || (props->downrate > 1 && !rate) || !srcfilter || (props->mime == "image/jpeg" && !decode) || !convert || !scalefilter || (props->show_clock && !clock) || (props->crop43 && !cropper) || !encode || !parse || !webrtc) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }

  // Verify resolution
  if (verify_resolution(props)) {
      RCLCPP_INFO(streamer_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate/props->framerate_denominator);
  } else {
      RCLCPP_ERROR(streamer_node->get_logger(), "Wrong resolution! Fallback pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate/props->framerate_denominator);
  };

  // 2. Set element properties
  set_source(source, props->device, props->io_mode);
  set_srcfilter(srcfilter, props->mime, props->width, props->height, props->framerate, props->framerate_denominator, props->downrate, props->brightness, props->contrast);
  set_convert(convert, props->chroma_resampler, props->dither, props->method);
  set_scalefilter(scalefilter, props->format, props->width, props->height, props->framerate, props->framerate_denominator, props->downscale, props->downrate, props->brightness, props->contrast);
  set_crop43(cropper, props->crop43, crop_width, props->downscale);
  set_x264(encode, props->tune, props->speed_preset, props->threads, props->bitrate, props->noise_reduction, props->gop, props->framerate, props->framerate_denominator, props->downrate);
  set_h264parse(parse, props->gop);
  set_webrtc(webrtc, props->serial, props->video_caps, props->do_fec, props->do_retransmission, props->congestion_control, props->bitrate);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, srcfilter, convert, scalefilter, encode, parse, webrtc, NULL);
  if (props->downrate > 1) gst_bin_add(GST_BIN(gst_pipeline), rate);
  if (props->crop43) gst_bin_add(GST_BIN(gst_pipeline), cropper);
  if (props->show_clock) gst_bin_add(GST_BIN(gst_pipeline), clock);
  if (props->mime == "image/jpeg") gst_bin_add(GST_BIN(gst_pipeline), decode);

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
  RCLCPP_DEBUG(streamer_node->get_logger(), "Getting props for %s", camera->serial.c_str());
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // Get profile
  std::string profile = "NULL";
  streamer_node->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + camera->serial + ".profile").c_str(), profile, profile);
  streamer_node->get_parameter_or<std::string>((std::string(DEFAULT_PREFIX) + "." + camera->original_serial + ".profile").c_str(), profile, profile);

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = set_property(streamer_node, camera->serial, profile, camera->original_serial, "device", props->node);
  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera->serial, profile, camera->original_serial, "io_mode", "mmap");

  // filter
  default_string = "NV12";
  props->format = set_property(streamer_node, camera->serial, profile, camera->original_serial, "format", default_string);
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera->serial, profile, camera->original_serial, "mime", default_string);

  props->brightness = set_property(streamer_node, camera->serial, profile, camera->original_serial, "brightness", 0);
  props->contrast = set_property(streamer_node, camera->serial, profile, camera->original_serial, "contrast", 0);
  props->framerate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate", 30);
  props->framerate_denominator = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate_denominator", 1);
  props->height = set_property(streamer_node, camera->serial, profile, camera->original_serial, "height", 720);
  props->width = set_property(streamer_node, camera->serial, profile, camera->original_serial, "width", 1280);

  // decoder
  default_string = "jpegdec";
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

  // rate
  props->downrate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "downrate", 1);

  // cropper
  props->crop43 = set_property(streamer_node, camera->serial, profile, camera->original_serial, "crop43", false);

  // clock
  props->show_clock = set_property(streamer_node, camera->serial, profile, camera->original_serial, "show_clock", false);

  // encode
  default_string = "ultrafast";
  props->speed_preset = set_property(streamer_node, camera->serial, profile, camera->original_serial, "speed_preset", default_string);
  default_string = "zerolatency";
  props->tune = set_property(streamer_node, camera->serial, profile, camera->original_serial, "tune", default_string);

  props->gop = set_property(streamer_node, camera->serial, profile, camera->original_serial, "gop", 1);
  props->noise_reduction = set_property(streamer_node, camera->serial, profile, camera->original_serial, "noise_reduction", 256);
  props->threads = set_property(streamer_node, camera->serial, profile, camera->original_serial, "threads", 1);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera->serial, profile, camera->original_serial, "congestion_control", default_string);
  props->video_caps = "video/x-h264";

  props->bitrate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "bitrate", 4096);

  props->do_fec = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_retransmission", false);

  return props;
}

/*
 * V4l camera (any) decoded then encoded into vpXenc
 * Enforces alignment from vpX v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-vp8
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
  GstElement* rate = (props->downrate > 1) ? gst_element_factory_make("videorate", "rate") : nullptr;
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* decode = (props->mime == "image/jpeg") ? gst_element_factory_make(props->decoder.c_str(), "decoder") : nullptr;
  GstElement* convert = gst_element_factory_make("videoconvertscale", "converter");
  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* encode = (props->video_caps == "video/x-vp9") ? gst_element_factory_make("vp9enc", "encoder") : gst_element_factory_make("vp8enc", "encoder");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");
  GstElement* clock = (props->show_clock) ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* cropper = (props->crop43) ? gst_element_factory_make("videocrop", "video-cropper") : nullptr;

  if (!gst_pipeline || !source || (props->downrate > 1 && !rate) || !srcfilter || (props->mime == "image/jpeg" && !decode) || !convert || !scalefilter  || (props->show_clock && !clock) || (props->crop43 && !cropper) || !encode || !webrtc) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }

  // Verify resolution
  if (verify_resolution(props)) {
      RCLCPP_INFO(streamer_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate/props->framerate_denominator);
  } else {
      RCLCPP_ERROR(streamer_node->get_logger(), "Wrong resolution! Fallback pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate/props->framerate_denominator);
  };
  
  // 2. Set element properties
  set_source(source, props->device, props->io_mode);
  set_srcfilter(srcfilter, props->mime, props->width, props->height, props->framerate, props->framerate_denominator, props->downrate, props->brightness, props->contrast);
  set_convert(convert, props->chroma_resampler, props->dither, props->method);
  set_scalefilter(scalefilter, props->format, props->width, props->height, props->framerate, props->framerate_denominator, props->downscale, props->downrate, props->brightness, props->contrast);
  set_crop43(cropper, props->crop43, crop_width, props->downscale);
  set_vpXenc(encode, props->deadline, props->cpu_used, props->end_usage, props->threads, props->bitrate, props->gop, props->framerate, props->framerate_denominator, props->downrate, props->video_caps, props->aq_mode);
  set_webrtc(webrtc, props->serial, props->video_caps, props->do_fec, props->do_retransmission, props->congestion_control, props->bitrate);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, srcfilter, convert, scalefilter, encode, webrtc, NULL);
  if (props->crop43) gst_bin_add(GST_BIN(gst_pipeline), cropper);
  if (props->show_clock) gst_bin_add(GST_BIN(gst_pipeline), clock);
  if (props->mime == "image/jpeg") gst_bin_add(GST_BIN(gst_pipeline), decode);
  if (props->downrate > 1) gst_bin_add(GST_BIN(gst_pipeline), rate);

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
  RCLCPP_DEBUG(streamer_node->get_logger(), "Getting props for %s", camera->serial.c_str());
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // Get profile
  std::string profile = "NULL";
  streamer_node->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + camera->serial + ".profile").c_str(), profile, profile);
  streamer_node->get_parameter_or<std::string>((std::string(DEFAULT_PREFIX) + "." + camera->original_serial + ".profile").c_str(), profile, profile);

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = set_property(streamer_node, camera->serial, profile, camera->original_serial, "device", props->node);
  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera->serial, profile, camera->original_serial, "io_mode", "mmap");

  // filter
  props->format = "I420";
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera->serial, profile, camera->original_serial, "mime", default_string);

  props->brightness = set_property(streamer_node, camera->serial, profile, camera->original_serial, "brightness", 0);
  props->contrast = set_property(streamer_node, camera->serial, profile, camera->original_serial, "contrast", 0);
  props->framerate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate", 30);
  props->framerate_denominator = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate_denominator", 1);
  props->height = set_property(streamer_node, camera->serial, profile, camera->original_serial, "height", 720);
  props->width = set_property(streamer_node, camera->serial, profile, camera->original_serial, "width", 1280);

  // decoder
  default_string = "jpegdec";
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

  // rate
  props->downrate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "downrate", 1);

  // cropper
  props->crop43 = set_property(streamer_node, camera->serial, profile, camera->original_serial, "crop43", false);

  // clock
  props->show_clock = set_property(streamer_node, camera->serial, profile, camera->original_serial, "show_clock", false);

  // encode
  default_string = "perceptual";
  props->aq_mode = set_property(streamer_node, camera->serial, profile, camera->original_serial, "aq_mode", default_string);
  default_string = "cbr";
  props->end_usage = set_property(streamer_node, camera->serial, profile, camera->original_serial, "end_usage", default_string);


  props->cpu_used = set_property(streamer_node, camera->serial, profile, camera->original_serial, "cpu_used", 16);
  props->deadline = set_property(streamer_node, camera->serial, profile, camera->original_serial, "deadline", 1);
  props->gop = set_property(streamer_node, camera->serial, profile, camera->original_serial, "gop", 1);
  props->threads = set_property(streamer_node, camera->serial, profile, camera->original_serial, "threads", 1);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera->serial, profile, camera->original_serial, "congestion_control", default_string);
  default_string = "video/x-vp8";
  props->video_caps = set_property(streamer_node, camera->serial, profile, camera->original_serial, "video_caps", default_string);

  props->bitrate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "bitrate", 4096);

  props->do_fec = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_retransmission", false);

  return props;
}

/*
 * V4l camera (any) decoded then encoded into av1enc
 * Enforces alignment from av1 v4l camera and feeds directly to webrtc 
 * gst-launch-1.0 v4l2src device={props->node} ! {props->mime},width={props->width},height={props->height},framerate={props->framerate}/1,alignment={props->alignment},stream-format={props->stream_format},format={props->format}! webrtcsink meta='meta, serial=(string){props->serial}' video-caps=video/x-av1
 */

GstElement* av1software_pipeline(rclcpp::Node* streamer_node, av1softwarePipelineProperties* props)
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
  GstElement* rate = (props->downrate > 1) ? gst_element_factory_make("videorate", "rate") : nullptr;
  GstElement* srcfilter = gst_element_factory_make("capsfilter", "srcfilter");
  GstElement* decode = (props->mime == "image/jpeg") ? gst_element_factory_make(props->decoder.c_str(), "decoder") : nullptr;
  GstElement* convert = gst_element_factory_make("videoconvertscale", "converter");
  GstElement* scalefilter = gst_element_factory_make("capsfilter", "scalefilter");
  GstElement* encode = gst_element_factory_make("av1enc", "encoder");
  GstElement* parse = gst_element_factory_make("av1parse", "parser");
  GstElement* webrtc = gst_element_factory_make("webrtcsink", "webrtc");
  GstElement* clock = (props->show_clock) ? gst_element_factory_make("clockoverlay", "clock") : nullptr;
  GstElement* cropper = (props->crop43) ? gst_element_factory_make("videocrop", "video-cropper") : nullptr;

  if (!gst_pipeline || !source || (props->downrate > 1 && !rate) || !srcfilter || (props->mime == "image/jpeg" && !decode) || !convert || !scalefilter  || (props->show_clock && !clock) || (props->crop43 && !cropper) || !encode || !parse || !webrtc) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not create pipeline for %s", props->serial.c_str());
      return nullptr;
  }

  // Verify resolution
  if (verify_resolution(props)) {
      RCLCPP_INFO(streamer_node->get_logger(), "Starting pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate/props->framerate_denominator);
  } else {
      RCLCPP_ERROR(streamer_node->get_logger(), "Wrong resolution! Fallback pipeline for %s with %dx%d@%dfps", props->serial.c_str(), props->width, props->height, props->framerate/props->framerate_denominator);
  };
  
  // 2. Set element properties
  set_source(source, props->device, props->io_mode);
  set_srcfilter(srcfilter, props->mime, props->width, props->height, props->framerate, props->framerate_denominator, props->downrate, props->brightness, props->contrast);
  set_convert(convert, props->chroma_resampler, props->dither, props->method);
  set_scalefilter(scalefilter, props->format, props->width, props->height, props->framerate, props->framerate_denominator, props->downscale, props->downrate, props->brightness, props->contrast);
  set_crop43(cropper, props->crop43, crop_width, props->downscale);
  set_av1enc(encode, props->cpu_used, props->end_usage, props->usage_profile, props->threads, props->bitrate, props->gop, props->framerate, props->framerate_denominator, props->downrate);
  set_webrtc(webrtc, props->serial, props->video_caps, props->do_fec, props->do_retransmission, props->congestion_control, props->bitrate);

  // 3. Add elements to pipeline
  gst_bin_add_many(GST_BIN(gst_pipeline), source, srcfilter, convert, scalefilter, encode, parse, webrtc, NULL);
  if (props->crop43) gst_bin_add(GST_BIN(gst_pipeline), cropper);
  if (props->show_clock) gst_bin_add(GST_BIN(gst_pipeline), clock);
  if (props->mime == "image/jpeg") gst_bin_add(GST_BIN(gst_pipeline), decode);
  if (props->downrate > 1) gst_bin_add(GST_BIN(gst_pipeline), rate);

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
 * Retrieve ros2 parameters for vpXsoftware pipeline or sets defaults
*/

av1softwarePipelineProperties* get_av1software_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera)
{
  // 0. Initialize constants
  av1softwarePipelineProperties* props = new av1softwarePipelineProperties;
  RCLCPP_DEBUG(streamer_node->get_logger(), "Getting props for %s", camera->serial.c_str());
  props->serial = camera->serial;
  props->node = camera->node;
  props->original_serial = camera->original_serial;

  // Get profile
  std::string profile = "NULL";
  streamer_node->get_parameter_or<std::string>((std::string(PIPELINE_PREFIX) + "." + camera->serial + ".profile").c_str(), profile, profile);
  streamer_node->get_parameter_or<std::string>((std::string(DEFAULT_PREFIX) + "." + camera->original_serial + ".profile").c_str(), profile, profile);

  // 1. Define default properties
  std::string default_string;

  // source
  props->device = set_property(streamer_node, camera->serial, profile, camera->original_serial, "device", props->node);
  default_string = "mmap";
  props->io_mode = set_property(streamer_node, camera->serial, profile, camera->original_serial, "io_mode", "mmap");

  // filter
  props->format = "I420";
  default_string = "image/jpeg";
  props->mime = set_property(streamer_node, camera->serial, profile, camera->original_serial, "mime", default_string);

  props->brightness = set_property(streamer_node, camera->serial, profile, camera->original_serial, "brightness", 0);
  props->contrast = set_property(streamer_node, camera->serial, profile, camera->original_serial, "contrast", 0);
  props->framerate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate", 30);
  props->framerate_denominator = set_property(streamer_node, camera->serial, profile, camera->original_serial, "framerate_denominator", 1);
  props->height = set_property(streamer_node, camera->serial, profile, camera->original_serial, "height", 720);
  props->width = set_property(streamer_node, camera->serial, profile, camera->original_serial, "width", 1280);

  // decoder
  default_string = "jpegdec";
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

  // rate
  props->downrate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "downrate", 1);

  // cropper
  props->crop43 = set_property(streamer_node, camera->serial, profile, camera->original_serial, "crop43", false);

  // clock
  props->show_clock = set_property(streamer_node, camera->serial, profile, camera->original_serial, "show_clock", false);

  // encode
  default_string = "cbr";
  props->end_usage = set_property(streamer_node, camera->serial, profile, camera->original_serial, "end_usage", default_string);
  default_string = "realtime";
  props->usage_profile = set_property(streamer_node, camera->serial, profile, camera->original_serial, "usage_profile", default_string);

  props->cpu_used = set_property(streamer_node, camera->serial, profile, camera->original_serial, "cpu_used", 10);
  props->gop = set_property(streamer_node, camera->serial, profile, camera->original_serial, "gop", 1);
  props->threads = set_property(streamer_node, camera->serial, profile, camera->original_serial, "threads", 1);

  // webrtc
  default_string = "gcc";
  props->congestion_control = set_property(streamer_node, camera->serial, profile, camera->original_serial, "congestion_control", default_string);
  props->video_caps = "video/x-av1";

  props->bitrate = set_property(streamer_node, camera->serial, profile, camera->original_serial, "bitrate", 4096);

  props->do_fec = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_fec", false);
  props->do_retransmission = set_property(streamer_node, camera->serial, profile, camera->original_serial, "do_retransmission", false);

  return props;
}

