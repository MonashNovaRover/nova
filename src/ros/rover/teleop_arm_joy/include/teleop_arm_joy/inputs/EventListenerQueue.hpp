//
// Created by nova on 1/3/70.
//

#ifndef EVENTLISTENERQUEUE_HPP
#define EVENTLISTENERQUEUE_HPP
#include <queue>
#include <rclcpp/time.hpp>

#include "EventListener.hpp"

namespace teleop_arm_joy {
/**
 * Any event listeners to be invoked are added to this queue, and the execution of EventListener::on_event_invoked is
 * deferred until teleop services the queue
 */
class EventListenerQueue {
public:
  /**
   * Clears the queue, after calling on_event_invoked for elements in the queue
   */
  void service(const rclcpp::Time& now);

  /**
   * Adds a listener to the queue to be invoked in the future when serviced.
   * @param listener A listener to have on_event_invoked called when serviced.
   */
  void enqueue(const EventListener::WeakPtr& listener);

private:
  std::queue<EventListener::WeakPtr> queue_{};
};

} // teleop_arm_joy

#endif //EVENTLISTENERQUEUE_HPP
