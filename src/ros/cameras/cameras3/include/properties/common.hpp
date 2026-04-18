#ifndef COMMON_PROPERTY_HEADER
#define COMMON_PROPERTY_HEADER

#include <string>
#include <stdlib.h>
#include <gst/gst.h>
#include <camera_msgs/msg/camera.hpp>
#include "pipelines/properties.hpp"

bool link_elements(rclcpp::Node* streamer_node, GstElement* first_element, GstElement* second_element, const std::string serial);

std::string set_property(rclcpp::Node* streamer_node, const camera_msgs::msg::Camera* camera, const std::string element, std::string value);

int set_property(rclcpp::Node* streamer_node, const camera_msgs::msg::Camera* camera, const std::string element, int value);

bool set_property(rclcpp::Node* streamer_node, const camera_msgs::msg::Camera* camera, const std::string element, bool value);

bool verify_v4lresolution(const std::string device_name, std::string* mime, int* width, int* height, int* framerate, int* framerate_denominator);

#endif
