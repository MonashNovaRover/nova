//
// Created by nova on 1/3/70.
//

#include <rclcpp/logging.hpp>
#include "teleop_arm_joy/inputs/events/EventListenerQueue.hpp"

namespace teleop_arm_joy {
void EventListenerQueue::service(const rclcpp::Time& now) {
  if (queue_.empty())
    return;

  while (!queue_.empty()) {
    auto listener_weak_ptr = (queue_.front());
    auto listener = listener_weak_ptr.lock();
    queue_.pop();

    if (listener) {
      listener->on_event_invoked(now);
    }
  }
}

void EventListenerQueue::enqueue(const EventListener::WeakPtr& listener) {
  queue_.push(listener);
}
} // teleop_arm_joy