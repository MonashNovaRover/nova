//
// Created by Bailey Chessum on 6/9/25.
//

#include "teleop_arm_joy/inputs/InputManager.hpp"

namespace teleop_arm_joy {
void InputManager::update(const rclcpp::Time& now) {
  // RCLCPP_INFO(rclcpp::get_logger("InputManager"), "update(%f)", now.seconds());

  for (auto& [name, boolean] : booleans_) {
    boolean->debounce(now);
  }

  for (auto& [name, axis] : axes_) {
    axis->debounce(now);
  }
}
} // namespace teleop_arm_joy
