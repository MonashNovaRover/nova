#include <string>
#include <gst/gst.h>
#define PIPELINE_PREFIX     "serial_pipelines"



struct Properties
{
  std::string serial;
  std::string node;
};

struct v4lProperties
{
  int width;
  int height;
  int framerate;
  int brightness;
  int contrast;
  std::string device;
  std::string mime;
  std::string v4l2_controls;
};

struct webRTCProperties
{
  std::string video_caps;
  bool do_fec;
  bool do_retransmission;
  std::string congestion_control;
};

struct h264passthroughProperties
{
  bool payload_quirk;
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
  std::string decoder;
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
};

struct v4l2webrtcPipelineProperties : Properties, v4lProperties, webRTCProperties, clockProperties {};
v4l2webrtcPipelineProperties* get_v4l2webrtc_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* v4l2webrtc_pipeline(rclcpp::Node* log_node, v4l2webrtcPipelineProperties* props);

struct h264passthroughPipelineProperties : Properties, v4lProperties, webRTCProperties, h264passthroughProperties {};
h264passthroughPipelineProperties* get_h264passthrough_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* h264passthrough_pipeline(rclcpp::Node* log_node, h264passthroughPipelineProperties* props);

struct h264softwarePipelineProperties : Properties, v4lProperties, webRTCProperties, x264encProperties {};
h264softwarePipelineProperties* get_h264software_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* h264software_pipeline(rclcpp::Node* log_node, h264softwarePipelineProperties* props);
