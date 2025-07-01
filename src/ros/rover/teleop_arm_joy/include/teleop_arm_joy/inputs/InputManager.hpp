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
  InputManager();
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
  InputCollection<Button> buttons_{};
  InputCollection<Axis> axes_{};

  EventCollection events_;
  std::shared_ptr<EventListenerQueue> event_listener_queue_ = nullptr;
};

} // teleop_arm_joy

#endif //INPUTMANAGER_HPP
