//
// Created by Bailey Chessum on 6/9/25.
//

#ifndef INPUTMANAGER_HPP
#define INPUTMANAGER_HPP

#include <rclcpp/node_interfaces/node_logging_interface.hpp>

#include "EventListenerQueue.hpp"
#include "collated/CollatedCollection.hpp"
#include "collated/CollatedInput.hpp"
#include "Input.hpp"
#include "EventCollection.hpp"

namespace teleop_arm_joy {

/**
 * Class responsible for owning the various maps between input name and input object.
 */
class InputManager {
public:
  InputManager();
  // Accessors
  [[nodiscard]] Collection<Input<double>>& get_axes() {
    return axes_;
  }
  [[nodiscard]] Collection<Input<bool>>& get_booleans() {
    return booleans_;
  }
  [[nodiscard]] EventCollection& get_events() {
    return events_;
  }

  void update(const rclcpp::Time& now);

protected:
  CollatedCollection<Input<double>, CollatedInput<double>> axes_{};
  CollatedCollection<Input<bool>, CollatedInput<bool>> booleans_{};

  EventCollection events_;
  std::shared_ptr<EventListenerQueue> event_listener_queue_ = nullptr;
};

} // teleop_arm_joy

#endif //INPUTMANAGER_HPP
