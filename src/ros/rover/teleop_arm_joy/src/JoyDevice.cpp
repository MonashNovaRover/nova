/**
 * @file JoyDevice.cpp
 * @brief Definitions for JoyDevice
 * Created 25/1/2025
 * Author: Bailey Chessum
 */

#include "teleop_arm_joy/JoyDevice.hpp"

teleop_arm_joy::JoyDevice::JoyDevice(const string &name, const vector<shared_ptr<JoyMessageListener>> &listeners) {
  this->name = name;
  this->listeners = listeners;
}

void teleop_arm_joy::JoyDevice::joyCallback(sensor_msgs::msg::Joy::SharedPtr joy_msg) {
  for (const auto& listener : this->listeners) {
    listener->joyCallback(joy_msg);
  }
}
