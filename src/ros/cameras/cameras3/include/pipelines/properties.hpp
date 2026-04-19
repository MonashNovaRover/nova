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
  bool verify_resolution;
};

struct capsProperties
{
  std::string format;
  std::string mime;

  int brightness;
  int contrast;
  int downrate;
  int height;
  int framerate;
  int framerate_denominator;
  int width;
};

struct h264PassthroughProperties
{
  bool payload_quirk;
};

struct softwareEncProperties
{
  std::string end_usage;
  std::string usage_profile;

  int cpu_used;
  int gop;
  int noise;
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

  int downscale;

  bool greyscale;

  bool crop43;
};

struct clockProperties
{
  bool show_clock;
};

struct Pipeline
{
  GstElement* gst_pipeline;
  Properties* props;
  camera_msgs::msg::Camera* camera;
};

struct av1softwarePipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, softwareEncProperties, cpuFiltersProperties, clockProperties, decodeProperties {};
av1softwarePipelineProperties* get_av1software_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* av1software_pipeline(rclcpp::Node* log_node, av1softwarePipelineProperties* props);

struct h264passthroughPipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, h264PassthroughProperties {};
h264passthroughPipelineProperties* get_h264passthrough_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* h264passthrough_pipeline(rclcpp::Node* log_node, h264passthroughPipelineProperties* props);

struct h264softwarePipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, softwareEncProperties, cpuFiltersProperties, clockProperties, decodeProperties {};
h264softwarePipelineProperties* get_h264software_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* h264software_pipeline(rclcpp::Node* log_node, h264softwarePipelineProperties* props);

struct vp8softwarePipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, softwareEncProperties, cpuFiltersProperties, clockProperties, decodeProperties {};
vp8softwarePipelineProperties* get_vp8software_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* vp8software_pipeline(rclcpp::Node* log_node, vp8softwarePipelineProperties* props);

struct vp9softwarePipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, softwareEncProperties, cpuFiltersProperties, clockProperties, decodeProperties {};
vp9softwarePipelineProperties* get_vp9software_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* vp9software_pipeline(rclcpp::Node* log_node, vp9softwarePipelineProperties* props);


struct v4lfallbackPipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, cpuFiltersProperties, clockProperties {};
v4lfallbackPipelineProperties* get_v4lfallback_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* v4lfallback_pipeline(rclcpp::Node* log_node, v4lfallbackPipelineProperties* props);

#endif
