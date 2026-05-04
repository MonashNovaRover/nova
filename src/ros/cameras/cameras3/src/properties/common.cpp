#include <string>
#include <stdlib.h>
#include <cstring>
#include <gst/gst.h>
#include "rclcpp/rclcpp.hpp"
#include <camera_msgs/msg/camera.hpp>
#include <systemd/sd-device.h>

#include "properties/common.hpp"
#include "pipelines/pipelines.hpp"

bool link_elements(rclcpp::Node* streamer_node, GstElement* first_element, GstElement* second_element, const std::string serial) {
  if (second_element != nullptr) {
    if (!gst_element_link(first_element, second_element)) {
      RCLCPP_ERROR(streamer_node->get_logger(), "Could not link %s to %s for %s", gst_object_get_name(GST_OBJECT(first_element)), gst_object_get_name(GST_OBJECT(second_element)), serial.c_str());
      return false;
    }
    return true;
  }
  return false;
}

std::string set_property(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera, const std::string element, std::string value) {
    // Check serial for property
    if (streamer_node->get_parameter<std::string>((std::string(PIPELINE_PREFIX) + "." + camera->serial + "." + element).c_str(), value)) return value;
    // Check profile for property
    if (!camera->profile.empty()) {
      if (streamer_node->get_parameter<std::string>((std::string(PROFILE_PREFIX) + "." + camera->original_serial + "." + camera->profile + "." + element).c_str(), value)) return value;
      if (streamer_node->get_parameter<std::string>((std::string(PROFILE_PREFIX) + "." + std::string(UNKNOWN_PROFILE_PREFIX) + "." + camera->profile + "." + element).c_str(), value)) return value;
    }
    // Check default for property
    if (streamer_node->get_parameter<std::string>((std::string(DEFAULT_PREFIX) + "." + camera->original_serial + "." + element).c_str(), value)) return value;
    return value;
}

int set_property(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera, const std::string element, int value) {
    // Check serial for property
    if (streamer_node->get_parameter((std::string(PIPELINE_PREFIX) + "." + camera->serial + "." + element).c_str(), value)) return value;
    // Check profile for property
    if (!camera->profile.empty()) {
      if (streamer_node->get_parameter((std::string(PROFILE_PREFIX) + "." + camera->original_serial + "." + camera->profile + "." + element).c_str(), value)) return value;
      if (streamer_node->get_parameter((std::string(PROFILE_PREFIX) + "." + std::string(UNKNOWN_PROFILE_PREFIX) + "." + camera->profile + "." + element).c_str(), value)) return value;
    }
    // Check default for property
    if (streamer_node->get_parameter((std::string(DEFAULT_PREFIX) + "." + camera->original_serial + "." + element).c_str(), value)) return value;
    return value;
}

float set_property(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera, const std::string element, float value) {
    // Check serial for property
    if (streamer_node->get_parameter((std::string(PIPELINE_PREFIX) + "." + camera->serial + "." + element).c_str(), value)) return value;
    // Check profile for property
    if (!camera->profile.empty()) {
      if (streamer_node->get_parameter((std::string(PROFILE_PREFIX) + "." + camera->original_serial + "." + camera->profile + "." + element).c_str(), value)) return value;
      if (streamer_node->get_parameter((std::string(PROFILE_PREFIX) + "." + std::string(UNKNOWN_PROFILE_PREFIX) + "." + camera->profile + "." + element).c_str(), value)) return value;
    }
    // Check default for property
    if (streamer_node->get_parameter((std::string(DEFAULT_PREFIX) + "." + camera->original_serial + "." + element).c_str(), value)) return value;
    return value;
}

bool set_property(rclcpp::Node* streamer_node, const std::unique_ptr<camera_msgs::msg::Camera>& camera, const std::string element, bool value) {
    // Check serial for property
    if (streamer_node->get_parameter((std::string(PIPELINE_PREFIX) + "." + camera->serial + "." + element).c_str(), value)) return value;
    // Check profile for property
    if (!camera->profile.empty()) {
      if (streamer_node->get_parameter((std::string(PROFILE_PREFIX) + "." + camera->original_serial + "." + camera->profile + "." + element).c_str(), value)) return value;
      if (streamer_node->get_parameter((std::string(PROFILE_PREFIX) + "." + std::string(UNKNOWN_PROFILE_PREFIX) + "." + camera->profile + "." + element).c_str(), value)) return value;
    }
    // Check default for property
    if (streamer_node->get_parameter((std::string(DEFAULT_PREFIX) + "." + camera->original_serial + "." + element).c_str(), value)) return value;
    return value;
}

void get_profile(rclcpp::Node* node, const std::unique_ptr<camera_msgs::msg::Camera>& camera) {
  // From serial
  if (node->get_parameter<std::string>((std::string(PIPELINE_PREFIX) + "." + camera->serial + ".profile").c_str(), camera->profile)) return;
  // From task profile
  std::string task, preset;
  if (node->get_parameter("task", task) && node->get_parameter("preset", preset)) {
    if (node->get_parameter<std::string>((std::string(PRESET_PREFIX) + "." + task + "." + preset + "." + camera->serial).c_str(), camera->profile)) return;
  }
  // From global
  if (!preset.empty()) {
    camera->profile = preset;
    return;
  }
  // From default
  if (node->get_parameter<std::string>((std::string(DEFAULT_PREFIX) + "." + camera->original_serial + ".profile").c_str(), camera->profile)) return;
  // If no profile found
  camera->profile = "";
}
