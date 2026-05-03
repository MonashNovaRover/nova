#ifndef PROPERTIES_HEADER
#define PROPERTIES_HEADER

#include <string>
#include <gst/gst.h>
#define PIPELINE_PREFIX         "serial_pipelines"
#define PROFILE_PREFIX          "profiles"
#define PRESET_PREFIX           "presets"
#define UNKNOWN_PROFILE_PREFIX  "unknown"
#define DEFAULT_PREFIX          "defaults"

struct Properties
{
  std::string serial;
  std::string node;
  std::string original_serial;
};

struct v4lProperties
{
  std::string device;
  std::string io_mode;

  bool strict_devname;
};

struct capsProperties
{
  std::string format;
  std::string mime;

  int brightness;
  int contrast;
  int downrate;
  int downscale;
  int height;
  int framerate;
  int framerate_denominator;
  int width;
};

struct h264PassthroughProperties
{
  bool payload_quirk;
};

struct glProperties
{
  float denoise_factor;
  float denoise_sigma;
  float denoise_threshold;
  int denoise_radius;
  float edgedetect_factor;
  float undistort_k1;
  float undistort_k2;
  float undistort_scale;

  float denoise;
  float edgedetect;
  bool undistort;
};

struct softwareEncProperties
{
  std::string end_usage;
  std::string usage_profile;

  int cpu_used;
  int deadline;
  int gop;
  int threads;
};

struct webRTCProperties
{
  std::string congestion_control;
  std::string video_caps;

  int bitrate;

  bool do_fec;
  bool do_retransmission;
};

struct decodeProperties
{

  std::string decoder;
  std::string jpegdec_method;
};

struct cpuFiltersProperties
{
  std::string chroma_resampler;
  std::string dither;
  std::string method;

  bool greyscale;
  bool crop43;
};

struct clockProperties
{
  bool show_clock;
};

struct rossinkProperties
{
  std::string ros_format;
  std::string ros_topic;

  bool rossink;
};


struct Pipeline
{
  GstElement* gst_pipeline;
  Properties* props;
  camera_msgs::msg::Camera* camera;
};

struct h264passthroughPipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, h264PassthroughProperties {};
h264passthroughPipelineProperties* get_h264passthrough_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* h264passthrough_pipeline(rclcpp::Node* log_node, h264passthroughPipelineProperties* props);

struct vpXsoftwarePipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, softwareEncProperties, cpuFiltersProperties, clockProperties, decodeProperties, rossinkProperties {};
vpXsoftwarePipelineProperties* get_vpXsoftware_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera, const int vpX);
GstElement* vpXsoftware_pipeline(rclcpp::Node* log_node, vpXsoftwarePipelineProperties* props, const int vpX);

struct vpXsoftwareGLPipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, softwareEncProperties, cpuFiltersProperties, clockProperties, decodeProperties, rossinkProperties, glProperties {};
vpXsoftwareGLPipelineProperties* get_vpXsoftwareGL_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera, const int vpX);
GstElement* vpXsoftwareGL_pipeline(rclcpp::Node* log_node, vpXsoftwareGLPipelineProperties* props, const int vpX, GstContext *display_ctx, GstContext *gl_ctx);

struct v4lfallbackPipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, cpuFiltersProperties, clockProperties {};
v4lfallbackPipelineProperties* get_v4lfallback_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* v4lfallback_pipeline(rclcpp::Node* log_node, v4lfallbackPipelineProperties* props);

#endif
