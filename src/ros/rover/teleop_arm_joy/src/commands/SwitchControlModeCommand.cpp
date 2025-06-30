//
// Created by nova on 6/29/25.
//

#include "../../include/teleop_arm_joy/commands/SwitchControlModeCommand.hpp"

namespace teleop_arm_joy {

void SwitchControlModeCommand::on_initialize(const std::string &prefix, const ParameterInterface::SharedPtr &parameters,
                                             CommandDelegate &context)
{
  Params params{};

  auto to_descriptor = rcl_interfaces::msg::ParameterDescriptor{};
  to_descriptor.name = prefix + "to";
  to_descriptor.description = "The name of the control mode to activate.";
  parameters->declare_parameter(to_descriptor.name, rclcpp::ParameterValue(""), to_descriptor);
  if (rclcpp::Parameter to_param; parameters->get_parameter(to_descriptor.name, to_param)) {
    params.to = to_param.as_string();

    // TODO: Make creating the control mode optional
    const auto control_modes = context.get_control_modes();
    control_modes->register_control_mode(const_cast<InputManager&>(context.get_inputs()), params.to);
  }

  params_ = params;
}

void SwitchControlModeCommand::execute(CommandDelegate& context, const rclcpp::Time& now) {
  context.get_control_modes()->set_control_mode(params_.to);
}

} // teleop_arm_joy

CLASS_LOADER_REGISTER_CLASS(teleop_arm_joy::SwitchControlModeCommand, teleop_arm_joy::Command);
