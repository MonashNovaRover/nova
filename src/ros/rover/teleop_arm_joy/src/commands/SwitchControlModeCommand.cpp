//
// Created by nova on 6/29/25.
//

#include "../../include/teleop_arm_joy/commands/SwitchControlModeCommand.hpp"
#include "teleop_arm_joy/control_modes/ControlModeManager.hpp"

namespace teleop_arm_joy {

void SwitchControlModeCommand::on_initialize(const std::string& prefix, const ParameterInterface::SharedPtr& parameters)
{
  Params params{};

  auto to_descriptor = rcl_interfaces::msg::ParameterDescriptor{};
  to_descriptor.name = prefix + "to";
  to_descriptor.description = "The name of the control mode to activate.";
  parameters->declare_parameter(to_descriptor.name, rclcpp::ParameterValue(""), to_descriptor);
  if (rclcpp::Parameter to_param; parameters->get_parameter(to_descriptor.name, to_param))
    params.to = to_param.as_string();

  params_ = params;
}

void SwitchControlModeCommand::execute(CommandDelegate& context, const rclcpp::Time& now) {
  context.get_control_modes()->set_control_mode(params_.to);
}

} // teleop_arm_joy

#include <pluginlib/class_list_macros.hpp>

CLASS_LOADER_REGISTER_CLASS(teleop_arm_joy::SwitchControlModeCommand, teleop_arm_joy::Command);
