#include "rclcpp/rclcpp.hpp"
#include "cameras/pipeline.hpp"
#include <camera_msgs/msg/camera.hpp>
#include <gst/gst.h>

#define SERVICE_START       "/camera_streamer/stream/start"
#define SERVICE_STOP        "/camera_streamer/stream/stop"
#define SERVICE_PAUSE       "/camera_streamer/stream/pause"
#define SERVICE_STATS       "/camera_streamer/stream/get_stats"
#define SERVICE_IPS         "/camera_streamer/get_host_ip"
#define SERVICE_DISCOVERY   "/camera_directory/discover"
#define TOPIC_CAMERAS       "/camera_directory/cameras"
#define POLLING_PERIOD      5000
#define PIPELINE_PREFIX     "serial_pipelines"

extern rclcpp::QoS discover_qos; 

struct Pipeline
{
  GstElement* gst_pipeline;
  Properties* props;
  camera_msgs::msg::Camera* camera;
};

struct v4l2webrtcPipelineProperties : Properties, v4lProperties, webRTCProperties, clockProperties {};
v4l2webrtcPipelineProperties* get_v4l2webrtc_pipeline_properties(rclcpp::Node* log_node, camera_msgs::msg::Camera* camera);
GstElement* v4l2webrtc_pipeline(rclcpp::Node* log_node, v4l2webrtcPipelineProperties* props);