//
// Created by Bailey Chessum on 10/6/25.
//

#include "teleop_arm_joy/input_sources/InputSource.hpp"

namespace teleop_arm_joy {

void InputSource::initialize(const std::shared_ptr<rclcpp::Node>& node, const std::string& name) {
  node_ = node;
  name_ = name;

  on_initialize();
}

void InputSource::configure(InputManager& inputs) {
  on_configure(inputs);
}

} // namespace teleop_arm_joy
