//
// Created by nova on 6/29/25.
//

#include "teleop_arm_joy/inputs/events/EventCollection.hpp"

namespace teleop_arm_joy {

Event::SharedPtr EventCollection::operator[](const std::string &index) {
  // Find the element
  const auto& it = events_.find(index);

  // Create a new event if it isn't in the collection
  if (it == events_.end()) {
    const auto new_item = std::make_shared<Event>(index, listener_queue_);

    events_[index] = new_item;
    return new_item;
  }

  return it->second;
}

EventCollection::iterator EventCollection::end() {
  return events_.end();
}

EventCollection::const_iterator EventCollection::end() const {
  return events_.end();
}

EventCollection::const_iterator EventCollection::begin() const {
  return events_.begin();
}

EventCollection::iterator EventCollection::begin() {
  return events_.begin();
}

} // teleop_arm_joy