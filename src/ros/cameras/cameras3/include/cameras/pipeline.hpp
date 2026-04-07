#include <string>
#include <gst/gst.h>
#define PIPELINE_PREFIX     "serial_pipelines"
#define PROFILE_PREFIX     "profiles"
#define DEFAULT_PREFIX     "defaults"

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
};

struct capsProperties
{
  std::string mime;

  int brightness;
  int contrast;
  int downrate;
  int height;
  int framerate;
  int framerate_denominator;
  int width;
};

struct h264passthroughProperties
{
  bool payload_quirk;
};

struct x264encProperties
{
  std::string me;
  std::string speed_preset;
  std::string tune;

  int bitrate;
  int gop;
  int noise_reduction;
  int subme;
  int threads;
};

struct vpXencProperties
{
  std::string end_usage;

  int bitrate;
  int cpu_used;
  int deadline;
  int gop;
  int threads;
};

struct webRTCProperties
{
  std::string congestion_control;
  std::string video_caps;

  bool do_fec;
  bool do_retransmission;
};

struct decodeProperties
{
  std::string decoder;
};

struct scaleProperties
{
  std::string chroma_resampler;
  std::string dither;
  std::string method;

  int downscale;
};

struct cropProperties
{
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
  std::string pipeline_type;
  camera_msgs::msg::Camera* camera;
  std::string profile;
  std::string original_serial;
};

struct v4l2webrtcPipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, scaleProperties, cropProperties, clockProperties {};
v4l2webrtcPipelineProperties* get_v4l2webrtc_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* v4l2webrtc_pipeline(rclcpp::Node* log_node, v4l2webrtcPipelineProperties* props);

struct h264passthroughPipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, h264passthroughProperties {};
h264passthroughPipelineProperties* get_h264passthrough_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* h264passthrough_pipeline(rclcpp::Node* log_node, h264passthroughPipelineProperties* props);

struct h264softwarePipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, x264encProperties, scaleProperties, cropProperties, clockProperties, decodeProperties {};
h264softwarePipelineProperties* get_h264software_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* h264software_pipeline(rclcpp::Node* log_node, h264softwarePipelineProperties* props);

struct vpXsoftwarePipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, vpXencProperties, scaleProperties, cropProperties, clockProperties, decodeProperties {};
vpXsoftwarePipelineProperties* get_vpXsoftware_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* vpXsoftware_pipeline(rclcpp::Node* log_node, vpXsoftwarePipelineProperties* props);
