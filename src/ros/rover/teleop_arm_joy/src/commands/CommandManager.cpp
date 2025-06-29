//
// Created by nova on 6/28/25.
//

#include "../../include/teleop_arm_joy/commands/CommandManager.hpp"

namespace teleop_arm_joy {

void CommandManager::create_command(const std::string& name, InputManager& inputs) {
  // TODO: Ensure the name not already defined
  const auto logger = node_->get_logger();

  std::string type;
  std::vector<std::string> invocation_event_names;

  if (!get_type_for_command(name, type, invocation_event_names))
    return;

  // Get the command class from pluginlib
  std::shared_ptr<Command> command_class = nullptr;

  try
  {
    command_class = loader_->createSharedInstance(type);
  }
  catch (const pluginlib::PluginlibException& ex)
  {
    RCLCPP_ERROR(logger, "Failed to find command plugin \"%s\" for command \"%s\"!\nwhat(): %s",
                 type.c_str(), name.c_str(), ex.what());
    return;
  }

  std::vector<Event::SharedPtr> events;
  for (const auto& invocation_event_name : invocation_event_names) {
    events.emplace_back(inputs.get_events()[invocation_event_name]);
  }

  command_class->initialize(context_, name, events, node_->get_node_logging_interface(), node_->get_node_parameters_interface());
  add(name, command_class);
}

void CommandManager::configure(InputManager& inputs) {
  const auto logger = node_->get_logger();

  // Declare and get parameter for control modes to spawn by default
  auto command_definitions_descriptor = rcl_interfaces::msg::ParameterDescriptor{};
  command_definitions_descriptor.name = "commands.names";
  command_definitions_descriptor.description = "The names of all commands to create, from parameters under "
                                               "commands.name.* for all names provided. Teleop won't know that "
                                               "parameters in the form commands.name.* exist if name is not in this "
                                               "array.";
  node_->declare_parameter(command_definitions_descriptor.name, rclcpp::ParameterValue(std::vector<std::string>()),
    command_definitions_descriptor);
  rclcpp::Parameter command_definitions_param;
  node_->get_parameter(command_definitions_descriptor.name, command_definitions_param);

  // Create each control mode according to the given params
  const auto command_names = command_definitions_param.get_type() == rclcpp::PARAMETER_STRING_ARRAY ?
     command_definitions_param.as_string_array() : std::vector<std::string>();

  // Pluginlib for loading commands dynamically
  loader_ = std::make_unique<pluginlib::ClassLoader<Command>>(
    "teleop_arm_joy", "teleop_arm_joy::Command");

  // List available input source plugins
  try
  {
    std::stringstream available_plugins_log{};
    const auto plugins = loader_->getDeclaredClasses();
    for (const auto& plugin : plugins) {
      available_plugins_log << "\n\t- " << plugin;
    }

    RCLCPP_DEBUG(logger, "Registered Command plugins:%s", available_plugins_log.str().c_str());
  }
  catch (const pluginlib::PluginlibException& ex)
  {
    RCLCPP_ERROR(logger, "Failed to list command plugins! what(): %s", ex.what());
    return;
  }

  for (const auto& name : command_names) {
    create_command(name, inputs);
  }
}

bool CommandManager::get_type_for_command(const std::string& name, std::string& source_type, std::vector<std::string>& invocation_event_names) const {
  // TODO: Check that the parameter hasn't already been defined
  // TODO: Remember that this parameter has already been defined

  // Declare parameter and get value for source_type
  auto type_descriptor = rcl_interfaces::msg::ParameterDescriptor{};
  type_descriptor.name = "commands." + name + ".type";
  type_descriptor.description = "The name of the plugin to load as the class for the \""+ name +"\" command";
  node_->declare_parameter(type_descriptor.name, rclcpp::ParameterType::PARAMETER_STRING, type_descriptor);
  rclcpp::Parameter type_param;
  const auto type_result = node_->get_parameter(type_descriptor.name, type_param);
  if (type_result)
    source_type = type_param.as_string();

  // Declare parameter and get value for invocation_event_names
  auto event_descriptor = rcl_interfaces::msg::ParameterDescriptor{};
  event_descriptor.name = "commands." + name + ".on";
  event_descriptor.description = "The name of the Event(s) that should cause the \""+ name +"\" command to execute. "
                                 "This could be the name of a button \"button\" to execute when pressed down, or "
                                 "\"button/up\" when released.";
  node_->declare_parameter(event_descriptor.name, rclcpp::ParameterType::PARAMETER_STRING, event_descriptor);
  rclcpp::Parameter event_param;
  const auto event_result = node_->get_parameter(event_descriptor.name, event_param);
  if (event_result) {
    if (event_param.get_type() == rclcpp::ParameterType::PARAMETER_STRING) {
      invocation_event_names = {event_param.as_string()};
    }
    else if (event_param.get_type() == rclcpp::PARAMETER_STRING_ARRAY) {
      invocation_event_names = event_param.as_string_array();
    }
    else {
      RCLCPP_ERROR(node_->get_logger(), "command.%s.on parameter must be a string or string array, but a %s was given",
        name.c_str(), event_param.get_type_name().c_str());
      invocation_event_names = std::vector<std::string>();
    }
  }

  return type_result && event_result;
}

std::shared_ptr<Command> CommandManager::operator[](const std::string& index) {
  return items_[index];
}

void CommandManager::add(const std::string& key, const std::shared_ptr<Command>& value) {
  items_.insert({key, value});
}
} // teleop_arm_joy