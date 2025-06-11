
#include "teleop_arm_joy/control_modes/ControlMode.hpp"

void teleop_arm_joy::ControlMode::initialize(const std::shared_ptr<rclcpp::Node>& node,
                                             const std::string& name)
{
  node_ = node;
  name_ = name;

  // Perform child class initialization
  on_initialize();
}

void teleop_arm_joy::ControlMode::configure(InputManager& inputs) {
  if (!node_) {
    return;
  }

  // Do common configuration

  // Perform child class configuration
  on_configure(inputs);
}

void teleop_arm_joy::ControlMode::activate() {
  if (!node_) {
    return;
  }

  on_activate();
}

void teleop_arm_joy::ControlMode::deactivate() {
  if (!node_) {
    return;
  }

  on_deactivate();
}
