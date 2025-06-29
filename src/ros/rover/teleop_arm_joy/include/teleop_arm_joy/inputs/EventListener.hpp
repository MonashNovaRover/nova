//
// Created by nova on 6/29/25.
//

#ifndef EVENTLISTENER_HPP
#define EVENTLISTENER_HPP
#include <memory>

namespace teleop_arm_joy {
/**
 * Interface providing a means for events to alert things that depend on them that they have been invoked.
 */
class EventListener {
public:
  virtual ~EventListener() = default;

  using WeakPtr = std::weak_ptr<EventListener>;
  using SharedPtr = std::shared_ptr<EventListener>;

  virtual void on_event_invoked(const rclcpp::Time& now) = 0;
};

} // teleop_arm_joy

#endif //EVENTLISTENER_HPP
