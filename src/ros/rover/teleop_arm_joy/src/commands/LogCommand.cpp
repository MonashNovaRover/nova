//
// Created by nova on 6/29/25.
//

#include "../../include/teleop_arm_joy/commands/LogCommand.hpp"
#include "colors.h"

namespace teleop_arm_joy {

void LogCommand::on_initialize(const std::string& prefix, const ParameterInterface::SharedPtr& parameters) {
  Params params{};

  auto message_descriptor = rcl_interfaces::msg::ParameterDescriptor{};
  message_descriptor.name = prefix + "message";
  message_descriptor.description = "The text to log.";
  parameters->declare_parameter(message_descriptor.name, rclcpp::ParameterValue(""), message_descriptor);
  if (rclcpp::Parameter message_param; parameters->get_parameter(message_descriptor.name, message_param))
    params.message = message_param.as_string();

  params_ = params;
}

void LogCommand::execute(CommandDelegate& context, const rclcpp::Time& now) {
  RCLCPP_INFO(get_logger(), "%s" C_RESET, params_.message.c_str());
}

} // teleop_arm_joy

#include <pluginlib/class_list_macros.hpp>

CLASS_LOADER_REGISTER_CLASS(teleop_arm_joy::LogCommand, teleop_arm_joy::Command);
