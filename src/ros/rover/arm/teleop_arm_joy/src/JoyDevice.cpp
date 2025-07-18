/**
 * @file JoyDevice.cpp
 * @brief Definitions for JoyDevice
 * Created 25/1/2025
 * Author: Bailey Chessum
 */

#include "teleop_arm_joy/JoyDevice.hpp"

teleop_arm_joy::JoyDevice::JoyDevice(rclcpp::Node* parent, const string& name, const Params::Devices::MapDeviceNames& config, const vector<shared_ptr<JoyMessageListener>>& listeners, const function<void(string&)>& callback) {
  this->name_ = name;
  this->listeners = listeners;
  this->callback = callback;
  this->parent_ = parent;

  joy_sub = parent->create_subscription<sensor_msgs::msg::Joy>(
    config.topic, rclcpp::QoS(10), std::bind(&JoyDevice::joyCallback, this, placeholders::_1));
}

void teleop_arm_joy::JoyDevice::debounce() {
  for (const auto& listener: this->listeners) {
    listener->debounce(parent_->now());
  }
}

void teleop_arm_joy::JoyDevice::joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
  for (const auto& listener: this->listeners) {
    listener->joyCallback(joy_msg);
  }

  // Alert other devices!
  callback(name_);
}
