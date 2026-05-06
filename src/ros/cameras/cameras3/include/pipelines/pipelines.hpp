#ifndef PIPELINES_HEADER
#define PIPELINES_HEADER

#include <memory>
#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"
#include <camera_msgs/msg/camera.hpp>

#include "pipelines/properties.hpp"

GstElement* h264passthrough_pipeline(rclcpp::Node* streamer_node, const std::unique_ptr<h264passthroughPipelineProperties>& props);
std::unique_ptr<h264passthroughPipelineProperties> get_h264passthrough_pipeline_properties(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera);

GstElement* vpXsoftware_pipeline(rclcpp::Node* streamer_node, const std::unique_ptr<vpXsoftwarePipelineProperties>& props, const int vpX);
std::unique_ptr<vpXsoftwarePipelineProperties> get_vpXsoftware_pipeline_properties(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera, const int vpX);

GstElement* vpXsoftwareGL_pipeline(rclcpp::Node* streamer_node, const std::unique_ptr<vpXsoftwareGLPipelineProperties>& props, const int vpX);
std::unique_ptr<vpXsoftwareGLPipelineProperties> get_vpXsoftwareGL_pipeline_properties(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera, const int vpX);
void set_vpXsoftwareGL_pipeline_properties(GstElement* gst_pipeline, const std::unique_ptr<vpXsoftwareGLPipelineProperties>& props, const int vpX);

GstElement* v4lfallback_pipeline(rclcpp::Node* streamer_node, const std::unique_ptr<v4lfallbackPipelineProperties>& props);
std::unique_ptr<v4lfallbackPipelineProperties> get_v4lfallback_pipeline_properties(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera);

#endif
