#ifndef PIPELINES_HEADER
#define PIPELINES_HEADER

#include "rclcpp/rclcpp.hpp"
#include <camera_msgs/msg/camera.hpp>

#include "pipelines/properties.hpp"

GstElement* av1software_pipeline(rclcpp::Node* streamer_node, av1softwarePipelineProperties* props);
av1softwarePipelineProperties* get_av1software_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera);

GstElement* h264passthrough_pipeline(rclcpp::Node* streamer_node, h264passthroughPipelineProperties* props);
h264passthroughPipelineProperties* get_h264passthrough_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera);

GstElement* h264software_pipeline(rclcpp::Node* streamer_node, h264softwarePipelineProperties* props);
h264softwarePipelineProperties* get_h264software_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera);

GstElement* vp8software_pipeline(rclcpp::Node* streamer_node, vp8softwarePipelineProperties* props);
vp8softwarePipelineProperties* get_vp8software_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera);

GstElement* vp8softwareGL_pipeline(rclcpp::Node* streamer_node, vp8softwareGLPipelineProperties* props);
vp8softwareGLPipelineProperties* get_vp8softwareGL_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera);

GstElement* vp9software_pipeline(rclcpp::Node* streamer_node, vp9softwarePipelineProperties* props);
vp9softwarePipelineProperties* get_vp9software_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera);

GstElement* v4lfallback_pipeline(rclcpp::Node* streamer_node, v4lfallbackPipelineProperties* props);
v4lfallbackPipelineProperties* get_v4lfallback_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera);

GstElement* v4lrostopic_pipeline(rclcpp::Node* streamer_node, v4lrostopicPipelineProperties* props);
v4lrostopicPipelineProperties* get_v4lrostopic_pipeline_properties(rclcpp::Node* streamer_node, camera_msgs::msg::Camera* camera);

#endif
