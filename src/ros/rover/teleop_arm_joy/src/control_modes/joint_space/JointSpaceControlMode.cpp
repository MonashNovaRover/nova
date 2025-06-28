//
// Created by Bailey Chessum on 6/7/25.
//

#include "teleop_arm_joy/control_modes/joint_space/JointSpaceControlMode.hpp"
#include "colors.h"

namespace teleop_arm_joy {

void JointSpaceControlMode::on_initialize() {
  param_listener_ = std::make_shared<joint_space_control_mode::ParamListener>(node_);
  params_ = param_listener_->get_params();
}

void JointSpaceControlMode::on_configure(InputManager& inputs) {
  if (param_listener_->is_old(params_))
    params_ = param_listener_->get_params();

  auto& input_axes = inputs.get_axes();
  for (auto axis_name : params_.axis_definitions) {
    axes_.push_back(input_axes[axis_name]);
  }

  auto& input_booleans = inputs.get_booleans();
  for (auto boolean_name : params_.button_definitions) {
    buttons_.push_back(input_booleans[boolean_name]);
  }
}

void JointSpaceControlMode::on_activate() {

}

void JointSpaceControlMode::on_deactivate() {

}

void JointSpaceControlMode::update() {
  auto logger = get_node()->get_logger();

  for (const auto& axis_ptr : axes_) {
    const auto axis = axis_ptr.lock();

    if (!axis)
      continue;

    if (axis->changed()) {
      RCLCPP_INFO(logger, C_INPUT "  %s\t%f", axis->get_name().c_str(), axis->value());
    }
  }

  for (const auto& boolean_ptr : buttons_) {
    const auto button = boolean_ptr.lock();

    if (!button)
      continue;

    if (button->changed()) {
      RCLCPP_INFO(logger, C_INPUT "  %s\t%d", button->get_name().c_str(), button->value());
    }
  }

}

}

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(teleop_arm_joy::JointSpaceControlMode, teleop_arm_joy::ControlMode);
