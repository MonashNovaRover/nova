/**
 * @file JoyAxis.cpp
 * @brief Definitions for JoyAxis
 * Created 25/1/2025
 * Author: Bailey Chessum
 */

#include "teleop_arm_joy/JoyAxis.hpp"

namespace
{
  constexpr auto AXIS_CHANGED_DIFFERECE = 0.05f;
}

teleop_arm_joy::JoyAxis::JoyAxis(const Params::Axes::MapAxisDefinitions &config) {
  id = config.id;
  invert = config.invert;
}

void teleop_arm_joy::JoyAxis::joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
  if (joy_msg->buttons.size() <= id)
    return;

  if (invert) {
    current_value_ = -joy_msg->axes[id];
  }
  else {
    current_value_ = joy_msg->axes[id];
  }
}

void teleop_arm_joy::JoyAxis::debounce(const rclcpp::Time& now) {
  last_debounce_value_ = current_debounce_value_;

  if (abs(last_debounce_value_ - current_value_) > AXIS_CHANGED_DIFFERECE) {
    current_debounce_value_ = current_value_;
  }
}

float teleop_arm_joy::JoyAxis::value() const {
  return current_value_;
}

bool teleop_arm_joy::JoyAxis::changed() const {
  return current_debounce_value_ != last_debounce_value_;
}
