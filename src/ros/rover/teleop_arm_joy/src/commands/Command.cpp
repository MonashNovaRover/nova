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
  on_initialize("commands." + name + ".", parameters);
}

// void Command::update(CommandDelegate& context, const rclcpp::Time& now) {
//   // RCLCPP_INFO(get_logger(), "%s checking %lu events for invocation", name_.c_str(), on_.size());
//   // TODO: Contemplate whether this should only be invoked once (I think it should be at the moment)
//   for (const auto& event : on_) {
//     RCLCPP_INFO(get_logger(), "%s Checking event %s (%d)", name_.c_str(), event->get_name().c_str(), event->is_invoked());
//     if (event->is_invoked()) {
//       RCLCPP_INFO(get_logger(), "Invoking %s", name_.c_str());
//       execute(context, now);
//       return;
//     }
//   }
// }

void Command::on_event_invoked(const rclcpp::Time& now) {
  if (const auto context = context_.lock())
    execute(*context, now);
}
} // teleop_arm_joy