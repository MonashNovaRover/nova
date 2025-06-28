//
// Created by nova on 6/28/25.
//

#include "../../include/teleop_arm_joy/commands/Command.hpp"

namespace teleop_arm_joy {
void Command::initialize(InputManager& inputs) {
  // TODO: parameterization to specify the event name as "on"
  on_ = inputs.get_events()["on"];
}

void Command::update(CommandDelegate& context) {
  if (on_->is_invoked())
    invoke(context);
}
} // teleop_arm_joy