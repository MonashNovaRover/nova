//
// Created by nova on 6/28/25.
//

#ifndef ACTION_HPP
#define ACTION_HPP
#include <rclcpp/node.hpp>

#include "CommandDelegate.hpp"

namespace teleop_arm_joy {

/**
 * Abstract base class for a generic invokable action (by an Event) that would change something in teleop
 */
class Command {

public:
  virtual ~Command() = default;

  Command() = default;

  void initialize(InputManager& inputs);
  void update(CommandDelegate& context);

  virtual void invoke(CommandDelegate& context) = 0;

protected:
  Event::SharedPtr on_;
};
} // teleop_arm_joy

#endif //ACTION_HPP
