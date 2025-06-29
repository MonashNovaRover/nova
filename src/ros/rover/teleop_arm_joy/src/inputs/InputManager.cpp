//
// Created by Bailey Chessum on 6/9/25.
//

#include "teleop_arm_joy/inputs/InputManager.hpp"

namespace teleop_arm_joy {
InputManager::InputManager() {
  event_listener_queue_ = std::make_shared<EventListenerQueue>();
  events_ = EventCollection(event_listener_queue_);
}

void InputManager::update(const rclcpp::Time& now) {
  for (auto& [name, boolean] : booleans_) {
    boolean->debounce(now);
  }

  for (auto& [name, axis] : axes_) {
    axis->debounce(now);
  }

  // Invoke events of the same name whenever a button is pressed
  for (auto& [name, button] : booleans_) {
    if (!button->changed())
      continue;

    if (button->value()) {
      events_[name]->invoke();
    }
    else {
      events_[name + "/up"]->invoke();
    }
  }

  // Update events
  for (auto& [name, event] : events_) {
    event->update();
  }

  event_listener_queue_->service(now);
}
} // namespace teleop_arm_joy
