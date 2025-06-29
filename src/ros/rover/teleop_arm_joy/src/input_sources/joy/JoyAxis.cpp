/**
 * @file JoyAxis.cpp
 * @brief Definitions for JoyAxis
 * Created 25/1/2025
 * Author: Bailey Chessum
 */

#include <cmath>

#include "../../../include/teleop_arm_joy/input_sources/joy/JoyAxis.hpp"

namespace
{
  constexpr auto AXIS_CHANGED_DIFFERECE = 0.05f;
}

teleop_arm_joy::JoyAxis::JoyAxis(const std::string& name, const joy_input_source::Params::Axes::MapAxisDefinitions &config) : Input(name) {
  id = config.id;
  invert = config.invert;

  button_id_negative_ = config.button_id_negative;
  button_id_positive_ = config.button_id_positive;
}

void teleop_arm_joy::JoyAxis::joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
  current_value_ = 0.0f;

  // Construct base axis value from message
  if (id >= 0 && joy_msg->axes.size() > id) {
    current_value_ += joy_msg->axes[id];
  }

  if (button_id_positive_ >= 0) {
    current_value_ += static_cast<float>(joy_msg->buttons[button_id_positive_]);
  }

  if (button_id_negative_ >= 0) {
    current_value_ -= static_cast<float>(joy_msg->buttons[button_id_negative_]);
  }

  // Clamp value between -1 and 1
  current_value_ = std::fmin(1.0f, std::fmax(-1.0f, current_value_));

  // Apply inversion
  if (invert) {
    current_value_ = -current_value_;
  }
}

void teleop_arm_joy::JoyAxis::debounce(const rclcpp::Time& now) {
  last_debounce_value_ = current_debounce_value_;

  if (abs(last_debounce_value_ - current_value_) > AXIS_CHANGED_DIFFERECE) {
    current_debounce_value_ = current_value_;
  }
}

double teleop_arm_joy::JoyAxis::value() {
  return current_value_;
}

bool teleop_arm_joy::JoyAxis::changed() const {
  return current_debounce_value_ != last_debounce_value_;
}
