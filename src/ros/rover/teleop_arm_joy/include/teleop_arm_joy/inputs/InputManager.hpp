//
// Created by Bailey Chessum on 6/9/25.
//

#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP

#include <rclcpp/node_interfaces/node_logging_interface.hpp>

#include "teleop_arm_joy/inputs/events/EventListenerQueue.hpp"
#include "teleop_arm_joy/inputs/events/EventCollection.hpp"
#include "teleop_arm_joy/inputs/Button.hpp"
#include "teleop_arm_joy/inputs/Axis.hpp"
#include "InputCollection.hpp"

namespace teleop_arm_joy {

/**
 * Class responsible for owning the various maps between input name and input object.
 */
class InputManager {
public:
  InputManager()
  : event_listener_queue_(std::make_shared<EventListenerQueue>()),
    events_(event_listener_queue_), buttons_(events_), axes_(events_)
  {}

  // Add move constructor
  InputManager(InputManager&& other) noexcept
    : event_listener_queue_(std::move(other.event_listener_queue_))
    , events_(std::move(other.events_))
    , buttons_(std::move(other.buttons_))
    , axes_(std::move(other.axes_)) {}

  // Add move assignment
  InputManager& operator=(InputManager&& other) noexcept {
    if (this != &other) {
      event_listener_queue_ = std::move(other.event_listener_queue_);
      events_ = std::move(other.events_);
      buttons_ = std::move(other.buttons_);
      axes_ = std::move(other.axes_);
    }
    return *this;
  }

  // Delete copy constructor and assignment
  InputManager(const InputManager&) = delete;
  InputManager& operator=(const InputManager&) = delete;

  // Accessors
  [[nodiscard]] InputCollection<Button>& get_buttons() {
    return buttons_;
  }
  [[nodiscard]] InputCollection<Axis>& get_axes() {
    return axes_;
  }
  [[nodiscard]] EventCollection& get_events() {
    return events_;
  }

  void update(const rclcpp::Time& now);

protected:
  // Note: Order of members is important for proper destruction:
  // 1. event_listener_queue_ must outlive events_
  // 2. events_ must outlive buttons_ and axes_
  std::shared_ptr<EventListenerQueue> event_listener_queue_;
  EventCollection events_{event_listener_queue_};
  InputCollection<Button> buttons_{events_};
  InputCollection<Axis> axes_{events_};
};

} // teleop_arm_joy

#endif //INPUTMANAGER_HPP
