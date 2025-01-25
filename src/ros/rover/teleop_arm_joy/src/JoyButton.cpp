/**
 * @file JoyButton.cpp
 * @brief Definitions for JoyButton
 * Created 25/1/2025
 * Author: Bailey Chessum
 */

#include "teleop_arm_joy/JoyButton.hpp"

teleop_arm_joy::JoyButton::JoyButton(Params::DeviceMappings::MapDevices::Buttons::MapButtonDefinitions config) {
  id = config.id;
}

void teleop_arm_joy::JoyButton::joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
  previousValue = currentValue;

  if (joy_msg->buttons.size() <= id)
    return;

  currentValue = joy_msg->buttons[id];
}

bool teleop_arm_joy::JoyButton::value() const {
  return currentValue;
}

bool teleop_arm_joy::JoyButton::down() const {
  return currentValue && !previousValue;
}

bool teleop_arm_joy::JoyButton::up() const {
  return !currentValue && previousValue;
}
