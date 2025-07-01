//
// Created by Bailey Chessum on 6/7/25.
//

#include "teleop_arm_joy/control_modes/twist/TwistControlMode.hpp"

namespace teleop_arm_joy {

void TwistControlMode::on_initialize() {
  param_listener_ = std::make_shared<twist_control_mode::ParamListener>(node_);
  params_ = param_listener_->get_params();
}

void TwistControlMode::on_configure(InputManager& inputs) {
  const auto logger = get_node()->get_logger();

  if (param_listener_->is_old(params_))
    params_ = param_listener_->get_params();

  // Create publisher
  const rclcpp::QoS qos_profile = rclcpp::QoS(
    rclcpp::QoSInitialization::from_rmw(rmw_qos_profile_sensor_data)
  ).best_effort().transient_local().keep_last(1);
  publisher_ = get_node()->create_publisher<geometry_msgs::msg::TwistStamped>(
    params_.topic, qos_profile);

  // Get interested inputs
  auto& axes = inputs.get_axes();
  auto& buttons = inputs.get_buttons();

  speed_coefficient_ = axes[params_.input_names.speed];
  locked_ = buttons[params_.input_names.locked];

  x_ = axes[params_.input_names.twist_x];
  y_ = axes[params_.input_names.twist_y];
  z_ = axes[params_.input_names.twist_z];
  roll_  = axes[params_.input_names.twist_roll];
  pitch_ = axes[params_.input_names.twist_pitch];
  yaw_   = axes[params_.input_names.twist_yaw];
}

void TwistControlMode::on_activate() {

}

void TwistControlMode::on_deactivate() {
  publish_halt_message(get_node()->now());
}

void TwistControlMode::publish_halt_message(const rclcpp::Time &now) const {
  auto msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
  msg->header.stamp = now;
  publisher_->publish(std::move(msg));
}

void TwistControlMode::update(const rclcpp::Time &now, const rclcpp::Duration &period) {
  auto logger = get_node()->get_logger();

  // Don't move when locked
  if (*locked_) {
    publish_halt_message(now);
    return;
  }

  const double speed_coefficient = std::clamp(speed_coefficient_->value(), 0.0, 1.0);
  auto msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
  msg->header.stamp = now;

  msg->twist.linear.x = x_->value() * speed_coefficient * params_.max_speed.linear;
  msg->twist.linear.y = y_->value() * speed_coefficient * params_.max_speed.linear;
  msg->twist.linear.z = z_->value() * speed_coefficient * params_.max_speed.linear;
  msg->twist.angular.x = roll_->value()  * speed_coefficient * params_.max_speed.angular;
  msg->twist.angular.y = pitch_->value() * speed_coefficient * params_.max_speed.angular;
  msg->twist.angular.z = yaw_->value()   * speed_coefficient * params_.max_speed.angular;

  if (params_.max_speed.normalized) {
    auto linear_input_norm = norm(x_->value(), y_->value(), z_->value());
    if (linear_input_norm > 1.0) {
      msg->twist.linear.x /= linear_input_norm;
      msg->twist.linear.y /= linear_input_norm;
      msg->twist.linear.z /= linear_input_norm;
    }

    auto angular_input_norm = norm(x_->value(), y_->value(), z_->value());
    if (angular_input_norm > 1.0) {
      msg->twist.angular.x /= angular_input_norm;
      msg->twist.angular.y /= angular_input_norm;
      msg->twist.angular.z /= angular_input_norm;
    }
  }

  msg->header.frame_id = params_.frame_id;

  publisher_->publish(std::move(msg));
}

double TwistControlMode::norm(double x, double y, double z) {
  return std::sqrt(x * x + y * y + z * z);
}


}

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(teleop_arm_joy::TwistControlMode, teleop_arm_joy::ControlMode);
