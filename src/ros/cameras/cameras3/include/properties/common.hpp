#ifndef COMMON_PROPERTY_HEADER
#define COMMON_PROPERTY_HEADER

#include <string>
#include <gst/gst.h>

bool link_elements(rclcpp::Node* streamer_node, GstElement* first_element, GstElement* second_element, const std::string serial);

std::string set_property(rclcpp::Node* streamer_node, const std::string serial, const std::string profile, const std::string original_serial, const std::string element, const std::string default_value);

int set_property(rclcpp::Node* streamer_node, const std::string serial, const std::string profile, const std::string original_serial, const std::string element, const int default_value);

bool set_property(rclcpp::Node* streamer_node, const std::string serial, const std::string profile, const std::string original_serial, const std::string element, const bool default_value);

bool verify_resolution(const std::string device_name, std::string* mime, int* width, int* height, int* framerate, int* framerate_denominator);

#endif
