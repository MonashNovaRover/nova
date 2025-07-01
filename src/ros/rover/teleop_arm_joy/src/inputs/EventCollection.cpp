//
// Created by nova on 6/29/25.
//

#include "teleop_arm_joy/inputs/events/EventCollection.hpp"

namespace teleop_arm_joy {


EventCollection::EventCollection(std::weak_ptr<EventListenerQueue> listener_queue)
  : listener_queue_(std::move(listener_queue)) {}

void EventCollection::clean_up() {
  for (auto it = items_.begin(); it != items_.end();) {
    if (it->second.expired()) {
      it = items_.erase(it);
    } else {
      ++it;
    }
  }
}

size_t EventCollection::size() const {
  size_t count = 0;
  for (const auto& item : items_) {
    if (!item.second.expired()) {
      ++count;
    }
  }
  return count;
}
} // teleop_arm_joy