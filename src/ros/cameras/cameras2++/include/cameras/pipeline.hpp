#include <string>
#include <gst/gst.h>
#define PIPELINE_PREFIX     "serial_pipelines"
#define PROFILE_PREFIX     "profiles"


struct Properties
{
  std::string serial;
  std::string node;
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
  int height;
  int framerate;
  int width;
};

struct webRTCProperties
{
  std::string congestion_control;
  std::string video_caps;

  bool do_fec;
  bool do_retransmission;
};

struct x264encProperties
{
  std::string tune;
  std::string speed_preset;
  std::string me;
  int subme;
  int threads;
  int bitrate;
  int noise_reduction;
  int gop;
};

struct vpXencProperties
{
  int deadline;
  int cpu_used;
  std::string end_usage;
  int threads;
  int bitrate;
  int gop;
};

struct decodeProperties
{
  std::string decoder;
};

struct cropProperties
{
  bool crop43;
};

struct h264passthroughProperties
{
  bool payload_quirk;
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
};

struct v4l2webrtcPipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, cropProperties, clockProperties {};
v4l2webrtcPipelineProperties* get_v4l2webrtc_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* v4l2webrtc_pipeline(rclcpp::Node* log_node, v4l2webrtcPipelineProperties* props);

struct h264passthroughPipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, h264passthroughProperties {};
h264passthroughPipelineProperties* get_h264passthrough_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* h264passthrough_pipeline(rclcpp::Node* log_node, h264passthroughPipelineProperties* props);

struct h264softwarePipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, x264encProperties, cropProperties, decodeProperties {};
h264softwarePipelineProperties* get_h264software_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* h264software_pipeline(rclcpp::Node* log_node, h264softwarePipelineProperties* props);

struct vpXsoftwarePipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, vpXencProperties, cropProperties, decodeProperties {};
vpXsoftwarePipelineProperties* get_vpXsoftware_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* vpXsoftware_pipeline(rclcpp::Node* log_node, vpXsoftwarePipelineProperties* props);
