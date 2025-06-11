//
// Created by Bailey Chessum on 6/7/25.
//

#include "teleop_arm_joy/control_modes/joint_space/TwistIKControlMode.hpp"

namespace teleop_arm_joy {

void TwistIKControlMode::on_initialize() {

}

void TwistIKControlMode::on_configure(InputManager& inputs) {

}

void TwistIKControlMode::on_activate() {

}

void TwistIKControlMode::on_deactivate() {

}

}

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(teleop_arm_joy::TwistIKControlMode, teleop_arm_joy::ControlMode);
