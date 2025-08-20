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

  return CallbackReturn::SUCCESS;
}

void JointSpaceControlMode::on_configure_inputs(Inputs inputs)
{
  const auto logger = get_node()->get_logger();

  // TODO: Implement a remapping functionality to avoid boilerplate parameters for names, like input source remapping
  speed_ = inputs.axes[params_.input_names.speed];

  joints_.clear();
  input_names_.clear();

  RCLCPP_DEBUG(logger, "Getting joint inputs");
  for (auto & [name, joint_params] : params_.joints.joint_definitions_map) {
    auto & [joint_name, scale, input_name] = joint_params;

    if (joint_name.empty())
      joint_name = name;

    if (input_name.empty())
      input_name = name;

    RCLCPP_DEBUG(logger, "  - %s -> %s", input_name.c_str(), joint_name.c_str());;

    if (scale == 0.0) {
      RCLCPP_WARN(logger, "joints.%s.scale isn't set! Values of 0 will always be sent for this joint.",
                  joint_name.c_str());
    }

    joints_.emplace_back(JointHandle{joint_name, inputs.axes[input_name], scale});
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

  RCLCPP_DEBUG(logger, "Calculating speed coefficient");
  const float speed_coefficient = params_.use_speed_input ? std::max(speed_->value(), 0.0f) : 1.0f;

  RCLCPP_DEBUG(logger, "Updating joint inputs");
  for (const auto& [name, axis, scale] : joints_) {
    RCLCPP_DEBUG(logger, "  [%s]", name.c_str());
  }

  RCLCPP_DEBUG(logger, "Updating joint inputs");
  for (const auto& [name, axis, scale] : joints_) {
    RCLCPP_DEBUG(logger, "  [%s]", name.c_str());

    if (!axis) {
      RCLCPP_ERROR(logger, "Axis::SharedPtr is missing for joint %s", name.c_str());
      continue;
    }

    msg->name.emplace_back(name);

    const float input = axis->value();

    RCLCPP_DEBUG(logger, "  - %s\t%f", name.c_str(), input);
    double velocity = static_cast<double>(input) * speed_coefficient * scale;

    RCLCPP_DEBUG(logger, "  - Putting velocity into message");
    msg->velocity.emplace_back(velocity);
  }

  RCLCPP_DEBUG(logger, "Publishing message");
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
