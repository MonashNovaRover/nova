//
// Created by Bailey Chessum on 6/7/25.
//

#include "teleop_arm_joy/control_modes/joint_space/JointSpaceControlMode.hpp"

namespace teleop_arm_joy {

void JointSpaceControlMode::on_initialize() {

}

void JointSpaceControlMode::on_configure(InputManager& inputs) {

}

void JointSpaceControlMode::on_activate() {

}

void JointSpaceControlMode::on_deactivate() {

}

void JointSpaceControlMode::update() {
  auto logger = get_node()->get_logger();

  RCLCPP_INFO(logger, "Update from %s!", get_node()->get_name());
}

}

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(teleop_arm_joy::JointSpaceControlMode, teleop_arm_joy::ControlMode);
