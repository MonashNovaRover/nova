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
void set_h264passthrough_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<h264passthroughPipelineProperties>& props);

struct rtsppassthroughPipelineProperties : Properties, rtspProperties, capsProperties, webRTCProperties {};
GstElement* rtsppassthrough_pipeline(rclcpp::Node* log_node, const std::unique_ptr<rtsppassthroughPipelineProperties>& props);
std::unique_ptr<rtsppassthroughPipelineProperties> get_rtsppassthrough_pipeline_properties(rclcpp::Node* node, const std::unique_ptr<camera_msgs::msg::Camera>& camera);
void set_rtsppassthrough_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<rtsppassthroughPipelineProperties>& props);

struct v4lsoftwarePipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, softwareEncProperties, cpuFiltersProperties, clockProperties, decodeProperties, rossinkProperties, zoomProperties {};
GstElement* v4lsoftware_pipeline(rclcpp::Node* log_node, const std::unique_ptr<v4lsoftwarePipelineProperties>& props, const int encoderX);
std::unique_ptr<v4lsoftwarePipelineProperties> get_v4lsoftware_pipeline_properties(rclcpp::Node* node, const std::unique_ptr<camera_msgs::msg::Camera>& camera, const int encoderX);
void set_v4lsoftware_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<v4lsoftwarePipelineProperties>& props);

struct v4lfallbackPipelineProperties : Properties, v4lProperties, capsProperties, webRTCProperties, cpuFiltersProperties, clockProperties, zoomProperties {};
GstElement* v4lfallback_pipeline(rclcpp::Node* log_node, const std::unique_ptr<v4lfallbackPipelineProperties>& props);
std::unique_ptr<v4lfallbackPipelineProperties> get_v4lfallback_pipeline_properties(rclcpp::Node* node, const std::unique_ptr<camera_msgs::msg::Camera>& camera);
void set_v4lfallback_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<v4lfallbackPipelineProperties>& props);

#endif
