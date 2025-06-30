//
// Created by nova on 6/28/25.
//

#include "../../include/teleop_arm_joy/commands/Command.hpp"

namespace teleop_arm_joy {

void Command::initialize(
  const CommandDelegate::WeakPtr& context,
  const std::string& name,
  const std::vector<Event::SharedPtr>& on,
  const LoggingInterface::SharedPtr& logging,
  const ParameterInterface::SharedPtr& parameters) {

  context_ = context;
  name_ = name;
  on_ = on;
  logger_ = logging->get_logger();

  // Do command implementation specific parameterization
  if (auto shared_context = context_.lock()) {
    on_initialize("commands." + name + ".", parameters, *shared_context);
  }
  else {
    RCLCPP_ERROR(get_logger(), "Command context is not available, when trying to initialize command %s.",
                 name_.c_str());
  }
}

void Command::on_event_invoked(const rclcpp::Time& now) {
  if (const auto context = context_.lock()) {
    execute(*context, now);
  }
  else {
    RCLCPP_ERROR(get_logger(), "Command context is not available, when trying to execute command %s.", name_.c_str());
  }
}
} // teleop_arm_joy