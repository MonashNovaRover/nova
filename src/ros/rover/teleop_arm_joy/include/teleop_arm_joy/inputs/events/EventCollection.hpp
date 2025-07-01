//
// Created by nova on 6/29/25.
//

#ifndef TELEOP_ARM_JOY_EVENTCOLLECTION_H
#define TELEOP_ARM_JOY_EVENTCOLLECTION_H

#include <utility>
#include <vector>
#include <memory>
#include <map>
#include "Event.hpp"
#include "../WeakMapIterator.hpp"

namespace teleop_arm_joy {

/**
 * A container of Events, where events that don't yet exist are created when an attempt is made to retrieve them.
 */
class EventCollection {
public:
  EventCollection() = default;
  explicit EventCollection(std::weak_ptr<EventListenerQueue> listener_queue);

  using iterator = WeakMapIterator<Event, false>;
  using const_iterator = WeakMapIterator<Event, true>;

  Event::SharedPtr operator[](const std::string& index) {
    auto it = items_.find(index);
    Event::SharedPtr ptr;

    if (it != items_.end()) {
      ptr = it->second.lock();
      if (!ptr) {
        items_.erase(it);
      }
    }

    if (!ptr) {
      ptr = std::make_shared<Event>(index, listener_queue_);
      items_[index] = ptr;
    }

    return ptr;
  }

  iterator begin() { return iterator(items_.begin(), &items_); }
  iterator end() { return iterator(items_.end(), &items_); }
  const_iterator begin() const { return const_iterator(items_.begin(), &items_); }
  const_iterator end() const { return const_iterator(items_.end(), &items_); }
  const_iterator cbegin() const { return const_iterator(items_.begin(), &items_); }
  const_iterator cend() const { return const_iterator(items_.end(), &items_); }

  void clean_up();

  /**
   * Gets the number of non-expired inputs
   */
  [[nodiscard]] size_t size() const;

private:
  std::map<std::string, std::weak_ptr<Event>> items_{};
  std::weak_ptr<EventListenerQueue> listener_queue_;
};

} // teleop_arm_joy

#endif //TELEOP_ARM_JOY_EVENTCOLLECTION_H
