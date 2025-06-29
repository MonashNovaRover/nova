//
// Created by Bailey Chessum on 4/6/25.
//

#ifndef EVENT_HPP
#define EVENT_HPP

#include <rclcpp/time.hpp>

#include "EventListener.hpp"

namespace teleop_arm_joy
{

class Event {
public:
  using SharedPtr = std::shared_ptr<Event>;

  explicit Event(const std::string& name) : name_(name) {}
  virtual ~Event() = default;

  virtual void invoke() {
    if (const auto listener_queue = listener_queue_.lock(); !invoked_ && listener_queue) {
      for (auto listener : listeners_) {
        listener_queue->enqueue(listener);
      }
    }

    invoked_ = true;
  }

  /**
   * Called to mark the end of the frame, and update the value of is_invoked()
   */
  void update() {
    previous_invoked_ = invoked_;
    invoked_ = false;
  }

  virtual bool is_invoked() {
    return previous_invoked_;
  }

  // Type conversion
  explicit operator bool() {
    return is_invoked();
  }

  // Accessors
  [[nodiscard]] const std::string& get_name() const {
    return name_;
  }

  using iterator = std::vector<std::weak_ptr<EventListener>>::iterator;
  using const_iterator = std::vector<std::weak_ptr<EventListener>>::const_iterator;

  iterator begin() {
    return listeners_.begin();
  }

  const_iterator begin() const {
    return listeners_.begin();
  }

  iterator end() {
    return listeners_.end();
  }

  const_iterator end() const {
    return listeners_.end();
  }

private:
  bool previous_invoked_ = false;
  bool invoked_ = false;

  std::string name_ = "uninitialized_event";

  /// Any Commands or other implementations of EventListener that should be notified when an Event is invoked.
  std::vector<EventListener::WeakPtr> listeners_;
  std::weak_ptr<EventListenerQueue> listener_queue_;
};

}

#endif // EVENT_HPP
