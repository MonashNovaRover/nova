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
  std::string mime;
  int bitrate;
};

struct webRTCProperties
{
  std::string video_caps;
  bool do_fec;
  bool do_retransmission;
  std::string congestion_control;
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

struct h264directPipelineProperties : Properties, v4lProperties, webRTCProperties {};
h264directPipelineProperties* get_h264direct_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* h264direct_pipeline(rclcpp::Node* log_node, h264directPipelineProperties* props);
