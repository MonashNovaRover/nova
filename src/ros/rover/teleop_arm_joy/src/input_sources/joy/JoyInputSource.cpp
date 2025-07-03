//
// Created by nova on 6/11/25.
//

#include "../../../include/teleop_arm_joy/input_sources/joy/JoyInputSource.hpp"

#include "colors.h"

namespace teleop_arm_joy {

void JoyInputSource::on_initialize() {
  param_listener_ = std::make_shared<joy_input_source::ParamListener>(node_);
  params_ = param_listener_->get_params();

  subscription_ = get_node()->create_subscription<sensor_msgs::msg::Joy>(
    params_.topic, rclcpp::QoS(10), std::bind(&JoyInputSource::joy_callback, this, std::placeholders::_1));
}

void JoyInputSource::export_buttons(std::vector<InputDeclaration<bool>>& declarations) {
  const auto logger = get_node()->get_logger();
  RCLCPP_DEBUG(logger, "Registered Buttons:");

  buttons_.clear();
  for (auto& [button_name, button_config] : params_.buttons.button_definitions_map) {
    // Buttons without a definition will have their value be -1 by default, so we can filter them out.
    if (button_config.id < 0)
      continue;

    RCLCPP_DEBUG(logger, "  %s", button_name.c_str());

    auto& button = buttons_.emplace_back(button_name, button_config);
    declarations.emplace_back(button.name, button.value);
  }
}

void JoyInputSource::export_axes(std::vector<InputDeclaration<double>>& declarations) {
  const auto logger = get_node()->get_logger();
  RCLCPP_DEBUG(logger, "Registered Axes:");

  axes_.clear();
  for (auto& [axis_name, axis_config] : params_.axes.axis_definitions_map) {
    // Axes without a definition will have their value be -1 by default, so we can filter them out.
    if (axis_config.id < 0 && axis_config.button_id_negative < 0 && axis_config.button_id_positive < 0)
      continue;

    RCLCPP_DEBUG(logger, "  %s", axis_name.c_str());

    auto& axis = axes_.emplace_back(axis_name, axis_config);
    declarations.emplace_back(axis.name, axis.value);
  }
}

void JoyInputSource::joy_callback(sensor_msgs::msg::Joy::SharedPtr msg) {
  std::unique_lock lock{joy_msg_mutex_};
  if (request_update(msg->header.stamp)) {
    joy_msg_ = msg;
  }
}

void JoyInputSource::on_update(const rclcpp::Time& now) {
  sensor_msgs::msg::Joy::SharedPtr joy_msg;

  { // Copy the pointer with mutex ownership
    std::unique_lock lock{joy_msg_mutex_};
    joy_msg = joy_msg_;
  }

  if (!joy_msg)
    return;

  // Apply button values
  for (auto& button : buttons_) {
    // Assume any buttons with ids < 0 have been filtered out already
    if (joy_msg->buttons.size() <= button.params.id)
      continue;

    button.value = joy_msg->buttons[button.params.id];
  }

  // Apply axis values
  for (auto& axis : axes_) {
    axis.value = 0.0;

    // Apply direct axis input
    if (axis.params.id >= 0 && joy_msg->axes.size() > axis.params.id) {
      axis.value = joy_msg->axes[axis.params.id];
    }

    // Apply axis from buttons
    if (axis.params.button_id_positive >= 0 && joy_msg->buttons.size() > axis.params.id) {
      if (joy_msg->buttons[axis.params.button_id_positive])
        axis.value += 1.0;
    }
    if (axis.params.button_id_negative >= 0 && joy_msg->buttons.size() > axis.params.id) {
      if (joy_msg->buttons[axis.params.button_id_negative])
        axis.value -= 1.0;
    }

    // Scale axis to fit min,max with a LERP
    const auto t = (axis.value + 1) * 0.5f;
    axis.value = t * (axis.params.max - axis.params.min) + axis.params.min;
  }
}

} // teleop_arm_joy

#include <pluginlib/class_list_macros.hpp>

CLASS_LOADER_REGISTER_CLASS(teleop_arm_joy::JoyInputSource, teleop_arm_joy::InputSource);
