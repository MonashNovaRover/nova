/**
 * @file JoyButton.cpp
 * @brief Definitions for JoyButton
 * Created 25/1/2025
 * Author: Bailey Chessum
 */

#include "teleop_arm_joy/JoyButton.hpp"

namespace
{
  constexpr auto BUTTON_DEBOUNCE_INTERVAL = std::chrono::milliseconds(20);
}

teleop_arm_joy::JoyButton::JoyButton(const Params::Buttons::MapButtonDefinitions &config) {
  id_ = config.id;
}

void teleop_arm_joy::JoyButton::joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
  if (joy_msg->buttons.size() <= id_)
    return;

  current_value_ = joy_msg->buttons[id_];
}

void teleop_arm_joy::JoyButton::debounce(const rclcpp::Time &now) {
  debounce_down_ = false;

  // Ignore if not press down
  if (!current_value_ || debounce_previous_value_) {
    debounce_previous_value_ = current_value_;
    return;
  }

  debounce_previous_value_ = current_value_;

  // Ignore if too soon since last press
  if (last_press_time_.has_value() && now - last_press_time_.value() < rclcpp::Duration(BUTTON_DEBOUNCE_INTERVAL))
    return;

  debounce_down_ = true;
}

bool teleop_arm_joy::JoyButton::value() const {
  return current_value_;
}

bool teleop_arm_joy::JoyButton::down() const {
  return debounce_down_;
}
