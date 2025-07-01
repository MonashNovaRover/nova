//
// Created by nova on 6/11/25.
//

#include "../../../include/teleop_arm_joy/input_sources/joy/JoyInputSource.hpp"

#include "colors.h"

namespace teleop_arm_joy {

void JoyInputSource::on_initialize() {
  param_listener_ = std::make_shared<joy_input_source::ParamListener>(node_);
  params_ = param_listener_->get_params();
}

void JoyInputSource::on_update(const rclcpp::Time& now) {
  InputSource::on_update(now);
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

} // teleop_arm_joy

#include <pluginlib/class_list_macros.hpp>

CLASS_LOADER_REGISTER_CLASS(teleop_arm_joy::JoyInputSource, teleop_arm_joy::InputSource);
