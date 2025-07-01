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

namespace teleop_arm_joy {

/**
 * A container of Events, where events that don't yet exist are created when an attempt is made to retrieve them.
 */
class EventCollection {
public:
  EventCollection() = default;
  explicit EventCollection(std::weak_ptr<EventListenerQueue> listener_queue) : listener_queue_(std::move(listener_queue)) {}

  /**
   * Finds the Event of the given name, creating an Event if one does not already exist.
   */
  Event::SharedPtr operator[](const std::string& index);

  using iterator = std::map<std::string, Event::SharedPtr>::iterator;
  using const_iterator = std::map<std::string, Event::SharedPtr>::const_iterator;

  iterator begin();
  [[nodiscard]] const_iterator begin() const;
  iterator end();
  [[nodiscard]] const_iterator end() const;

private:
  std::map<std::string, Event::SharedPtr> events_{};
  std::weak_ptr<EventListenerQueue> listener_queue_;
};

} // teleop_arm_joy

#endif //TELEOP_ARM_JOY_EVENTCOLLECTION_H
