#ifndef PIPELINES_HEADER
#define PIPELINES_HEADER

#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"
#include <camera_msgs/msg/camera.hpp>

#include "pipelines/properties.hpp"

GstElement* h264passthrough_pipeline(rclcpp::Node* streamer_node, h264passthroughPipelineProperties* props);
h264passthroughPipelineProperties* get_h264passthrough_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera);

GstElement* vpXsoftware_pipeline(rclcpp::Node* streamer_node, vpXsoftwarePipelineProperties* props, const int vpX);
vpXsoftwarePipelineProperties* get_vpXsoftware_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera, const int vpX);

GstElement* vpXsoftwareGL_pipeline(rclcpp::Node* streamer_node, vpXsoftwareGLPipelineProperties* props, const int vpX, GstContext *display_ctx, GstContext *gl_ctx);
vpXsoftwareGLPipelineProperties* get_vpXsoftwareGL_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera, const int vpX);

GstElement* v4lfallback_pipeline(rclcpp::Node* streamer_node, v4lfallbackPipelineProperties* props);
v4lfallbackPipelineProperties* get_v4lfallback_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera);

#endif
