#ifndef PIPELINES_HEADER
#define PIPELINES_HEADER

#include <memory>
#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"
#include <camera_msgs/msg/camera.hpp>

#include "pipelines/properties.hpp"

struct h264passthroughPipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, h264PassthroughProperties {};
GstElement* h264passthrough_pipeline(rclcpp::Node* log_node, const std::unique_ptr<h264passthroughPipelineProperties>& props);
std::unique_ptr<h264passthroughPipelineProperties> get_h264passthrough_pipeline_properties(rclcpp::Node* node, const std::unique_ptr<camera_msgs::msg::Camera>& camera);

struct vpXsoftwarePipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, softwareEncProperties, cpuFiltersProperties, clockProperties, decodeProperties, rossinkProperties, glProperties {};
GstElement* vpXsoftware_pipeline(rclcpp::Node* log_node, const std::unique_ptr<vpXsoftwarePipelineProperties>& props, const int vpX);
std::unique_ptr<vpXsoftwarePipelineProperties> get_vpXsoftware_pipeline_properties(rclcpp::Node* node, const std::unique_ptr<camera_msgs::msg::Camera>& camera, const int vpX);
void set_vpXsoftware_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<vpXsoftwarePipelineProperties>& props);

struct v4lfallbackPipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, cpuFiltersProperties, clockProperties {};
GstElement* v4lfallback_pipeline(rclcpp::Node* log_node, const std::unique_ptr<v4lfallbackPipelineProperties>& props);
std::unique_ptr<v4lfallbackPipelineProperties> get_v4lfallback_pipeline_properties(rclcpp::Node* node, const std::unique_ptr<camera_msgs::msg::Camera>& camera);
void set_v4lfallback_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<v4lfallbackPipelineProperties>& props);

#endif
