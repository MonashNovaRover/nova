#ifndef COMMON_PROPERTY_HEADER
#define COMMON_PROPERTY_HEADER

#include <string>
#include <stdlib.h>
#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"
#include <camera_msgs/msg/camera.hpp>
#include "pipelines/properties.hpp"
#include "cameras/colors.hpp"

bool link_elements(rclcpp::Node* streamer_node, GstElement* first_element, GstElement* second_element, const std::string serial);

std::string set_property(rclcpp::Node* streamer_node, const camera_msgs::msg::Camera* camera, const std::string element, std::string value);

int set_property(rclcpp::Node* streamer_node, const camera_msgs::msg::Camera* camera, const std::string element, int value);

float set_property(rclcpp::Node* streamer_node, const camera_msgs::msg::Camera* camera, const std::string element, float value);

bool set_property(rclcpp::Node* streamer_node, const camera_msgs::msg::Camera* camera, const std::string element, bool value);

template<typename properties> void display_resolution(rclcpp::Node* streamer_node, const properties props, camera_msgs::msg::Camera* camera, const int crop_width) {
  RCLCPP_INFO(streamer_node->get_logger(), "%sInitialized pipeline: %s%s%s for %s%s%s with profile: %s%s %dx%d@%.2gfps%s", C_QUIET, C_INPUT, camera->pipeline_type.c_str(), C_QUIET, C_TITLE, props->serial.c_str(), C_QUIET, C_MODE, camera->profile.c_str(), (int)(((float)props->width-(float)2*crop_width)/(float)props->downscale), (int)((float)props->height/(float)props->downscale), (double) props->framerate/props->framerate_denominator/props->downrate, C_RESET);
}

void get_profile(rclcpp::Node* node, camera_msgs::msg::Camera* camera);

#endif
