// Copyright 2025 Bailey Chessum
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
#include "joint_space_control_mode/joint_space_control_mode.hpp"

namespace
{
// This value is used to avoid normalized limits that would cause a divide by zero
constexpr double EPSILON = 1e-6;
}

namespace joint_space_control_mode
{

JointSpaceControlMode::JointSpaceControlMode() = default;

JointSpaceControlMode::~JointSpaceControlMode() = default;

return_type JointSpaceControlMode::on_init()
{
  param_listener_ = std::make_shared<joint_space_control_mode::ParamListener>(get_node());

  return return_type::OK;
}

CallbackReturn JointSpaceControlMode::on_configure(const State &)
{
  const auto logger = get_node()->get_logger();

  // Get the parameters (if they changed)
  if (param_listener_->is_old(params_)) {
    params_ = param_listener_->get_params();
  }

  // Create the publishers based on the params we just got

  if (!params_.topic.empty()) {
    publisher_ = get_node()->create_publisher<nova_interfaces::msg::ArmFkVelocityTargets>(
      params_.topic, params_.qos);
  }
  else {
    RCLCPP_ERROR(get_node()->get_logger(), "The topic parameter isn't set!");
    return CallbackReturn::ERROR;
  }

  // Get params for every joint
  bool has_params = true;
  int i = 0;
  do {
    const std::string i_str = std::to_string(i);
    std::string prefix = "joints." + i_str + ".";
    std::string name;

    if (const bool has_name = get_node()->get_parameter<std::string>(prefix + "name", name); !has_name) {
      has_params = i == 0;
      i++;
      continue;
    }

    double scale = 0.0;
    std::string input_name;

    get_node()->get_parameter_or<double>(prefix + "scale", scale, 0.0);
    get_node()->get_parameter_or<std::string>(prefix + "input_name", input_name, "j" + i_str);

    joints_.emplace_back(JointHandle{
      name,
      nullptr,
      scale
    });
    input_names_.push_back(input_name);

    i++;
  } while (has_params);

  return CallbackReturn::SUCCESS;
}

void JointSpaceControlMode::on_capture_inputs(Inputs inputs)
{
  // TODO: Implement a remapping functionality to avoid boilerplate parameters for names, like input source remapping
  speed_ = inputs.axes[params_.input_names.speed];

  for (size_t i = 0; i < input_names_.size(); ++i) {
    const auto & input_name = input_names_[i];
    auto & joint = joints_[i];

    joint.axis = inputs.axes[input_name];
  }
}

CallbackReturn JointSpaceControlMode::on_activate(const State &)
{
  return CallbackReturn::SUCCESS;
}

CallbackReturn JointSpaceControlMode::on_deactivate(const State &)
{
  publish_halt_message(get_node()->now());

  return CallbackReturn::SUCCESS;
}

void JointSpaceControlMode::publish_halt_message(const rclcpp::Time & now) const
{
  if (publisher_) {
    auto msg = std::make_unique<nova_interfaces::msg::ArmFkVelocityTargets>();
    msg->header.stamp = now;
    publisher_->publish(std::move(msg));
  }
}

return_type JointSpaceControlMode::on_update(const rclcpp::Time & now, const rclcpp::Duration &)
{
  auto logger = get_node()->get_logger();

  // Don't move when locked
  if (is_locked()) {
    publish_halt_message(now);
    return return_type::OK;
  }


  auto msg = std::make_unique<nova_interfaces::msg::ArmFkVelocityTargets>();
  msg->header.stamp = now;

  const float speed_coefficient = params_.use_speed_input ? std::max(speed_->value(), 0.0f) : 1.0f;

  for (const auto& [name, axis, scale] : joints_) {
    msg->name.emplace_back(name);

    const float input = axis->value();
    double velocity = static_cast<double>(input) * speed_coefficient * scale;

    msg->velocity.emplace_back(velocity);
  }

  if (publisher_) {
    publisher_->publish(std::move(msg));
  }

  return return_type::OK;
}

CallbackReturn JointSpaceControlMode::on_error(const State &)
{
  return CallbackReturn::SUCCESS;
}

CallbackReturn JointSpaceControlMode::on_cleanup(const State &)
{
  return CallbackReturn::SUCCESS;
}

CallbackReturn JointSpaceControlMode::on_shutdown(const State &)
{
  locked_.reset();
  speed_.reset();

  joints_.clear();

  publisher_.reset();

  return CallbackReturn::SUCCESS;
}

}  // namespace joint_space_control_mode

#include <pluginlib/class_list_macros.hpp>

PLUGINLIB_EXPORT_CLASS(joint_space_control_mode::JointSpaceControlMode, control_mode::ControlMode);
