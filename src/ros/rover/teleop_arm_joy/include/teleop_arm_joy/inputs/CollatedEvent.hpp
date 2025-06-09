//
// Created by nova on 6/9/25.
//

#ifndef COLLATEDEVENT_HPP
#define COLLATEDEVENT_HPP

#include "Event.hpp"

namespace teleop_arm_joy {
/**
 * This is the class type actually held by the EventManager, which combines multiple other input types.
 */
class CollatedEvent final : public Event {
public:
  void invoke() override {
    for (auto event : events_) {
      if (event)
        event->invoke();
    }
  };

  void add(const std::shared_ptr<Event>& event) {
    events_.emplace_back(event);
  }

  void remove(const std::shared_ptr<Event>& event) {
    // Find the element to remove
    const auto it = std::find(events_.begin(), events_.end(), event);

    // Do nothing if not in events_
    if (it == events_.end())
      return;

    // TODO: Do this by swapping with the end and popping to be O(1) rather than O(N)
    events_.erase(it);
  }

private:
  std::vector<std::shared_ptr<Event>> events_ = {};
};

} // teleop_arm_joy

#endif //COLLATEDEVENT_HPP
