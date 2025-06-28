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
  const auto logger = get_node()->get_logger();

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

  // Create publisher
  const rclcpp::QoS qos_profile = rclcpp::QoS(
    rclcpp::QoSInitialization::from_rmw(rmw_qos_profile_sensor_data)
  ).best_effort().transient_local().keep_last(1);
  publisher_ = get_node()->create_publisher<nova_interfaces::msg::ArmFkVelocityTargets>(
    params_.topic, qos_profile);

  // Get interested inputs
  locked_ = inputs.get_booleans()[params_.locked_input_name];
  speed_coefficient_ = inputs.get_axes()[params_.speed_input_name];

  const auto joint_count = params_.joints.joint_definitions_map.size();
  joints_.clear();
  joints_.reserve(joint_count);

  for (const auto& joint_name : params_.joint_definitions) {
    const auto it = params_.joints.joint_definitions_map.find(joint_name);

    if (it == params_.joints.joint_definitions_map.end()) {
      RCLCPP_ERROR(logger, "Joint \"%s\" was defined in the \"joint_definitions\" parameter, but you didn't specify any"
                           " values for \"joints.%s\"", joint_name.c_str(), joint_name.c_str());
      continue;
    }

    const auto joint_config = it->second;
    const auto input_name = joint_config.input_name.empty() ? joint_name : joint_config.input_name;
    const auto input = inputs.get_axes()[input_name];

    joints_.emplace_back(JointHandle{
      joint_name,
      joint_config,
      input
    });
  }
}

void JointSpaceControlMode::on_activate() {

}

void JointSpaceControlMode::on_deactivate() {
  publish_halt_message(get_node()->now());
}

void JointSpaceControlMode::publish_halt_message(const rclcpp::Time& now) const {
  auto msg = std::make_unique<nova_interfaces::msg::ArmFkVelocityTargets>();
  msg->header.stamp = now;

  for (const auto& joint : joints_) {
    msg->name.emplace_back(joint.name);
    msg->velocity.emplace_back(0.0);
  }

  publisher_->publish(std::move(msg));
}

void JointSpaceControlMode::update(const rclcpp::Time& now, const rclcpp::Duration& period) {
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

  // Don't move when locked
  if (*locked_) {
    publish_halt_message(now);
    return;
  }

  // Publish a message
  const double speed_coefficient = std::clamp(speed_coefficient_->value(), 0.0, 1.0);
  auto msg = std::make_unique<nova_interfaces::msg::ArmFkVelocityTargets>();
  msg->header.stamp = now;

  for (const auto& [name, config, input] : joints_) {
    msg->name.emplace_back(name);

    const auto input_speed = *input * speed_coefficient * config.max_speed;
    msg->velocity.emplace_back(input_speed);
  }

  publisher_->publish(std::move(msg));
}

}

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(teleop_arm_joy::JointSpaceControlMode, teleop_arm_joy::ControlMode);
