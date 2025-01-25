/**
 * @file JoyAxis.cpp
 * @brief Definitions for JoyAxis
 * Created 25/1/2025
 * Author: Bailey Chessum
 */

#include "teleop_arm_joy/JoyAxis.hpp"

teleop_arm_joy::JoyAxis::JoyAxis(const Params::DeviceMappings::MapDevices::Axes::MapAxisDefinitions config) {
  id = config.id;
}

void teleop_arm_joy::JoyAxis::joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
  if (joy_msg->buttons.size() <= id)
    return;

  currentValue = joy_msg->axes[id];
}

float teleop_arm_joy::JoyAxis::value() const {
  return currentValue;
}
