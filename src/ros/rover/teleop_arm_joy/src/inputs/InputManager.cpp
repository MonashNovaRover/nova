//
// Created by Bailey Chessum on 6/9/25.
//

#include "teleop_arm_joy/inputs/InputManager.hpp"

namespace teleop_arm_joy {

void InputManager::update(const rclcpp::Time& now) {
  for (auto& button : buttons_) {
    button->debounce(now);
  }

  for (auto& axis : axes_) {
    axis->debounce(now);
  }

  // Update events
  for (const auto& button : buttons_) {
    button->update_events(now);
  }
  for (const auto& axis : axes_) {
    axis->update_events(now);
  }
  for (auto& event : events_) {
    event->update();
  }

  event_listener_queue_->service(now);
}
} // namespace teleop_arm_joy
