//
// Created by nova on 6/11/25.
//

#include "teleop_arm_joy/input_sources/InputSourceManager.hpp"

namespace teleop_arm_joy {

void InputSourceManager::configure() {
  const auto logger = node_->get_logger();

  // Declare and get parameter for control modes to spawn by default
  node_->declare_parameter("input_sources", rclcpp::ParameterType::PARAMETER_STRING_ARRAY);
  rclcpp::Parameter input_sources_param;
  node_->get_parameter("input_sources", input_sources_param);
  RCLCPP_INFO(logger, "input_sources param is of type %s", input_sources_param.get_type_name().c_str());

  // Pluginlib for loading control modes dynamically
  source_loader_ = std::make_unique<pluginlib::ClassLoader<InputSource>>(
    "teleop_arm_joy", "teleop_arm_joy::InputSource");

  // List available control mode plugins
  try
  {
    RCLCPP_INFO(logger, "Available input source plugins:");
    const auto plugins = source_loader_->getDeclaredClasses();
    for (const auto& plugin : plugins) {
      RCLCPP_INFO(logger, "  - %s", plugin.c_str());
    }
  }
  catch (const pluginlib::PluginlibException& ex)
  {
    RCLCPP_ERROR(logger, "Failed to list input source plugins! what(): %s", ex.what());
    return;
  }

  // Create each control mode according to the given params
  const auto input_source_names = input_sources_param.as_string_array();
  for (auto& input_source_name : input_source_names) {
    std::string input_source_type;

    // Get the control mode's plugin type name
    if (!get_type_for_input_source(input_source_name, input_source_type)) {
      RCLCPP_ERROR(logger, "Failed to find type for input source \"%s\" in params. Have you defined %s.type in your "
                           "parameter file?", input_source_name.c_str(), input_source_name.c_str());
      continue;
    }

    // Get the control mode class from pluginlib
    std::shared_ptr<InputSource> input_source_class = nullptr;
    try
    {
      input_source_class = source_loader_->createSharedInstance(input_source_type);
    }
    catch (const pluginlib::PluginlibException& ex)
    {
      RCLCPP_ERROR(logger, "Failed to find input source plugin \"%s\" for mode \"%s\"!\nwhat(): %s",
                   input_source_type.c_str(), input_source_name.c_str(), ex.what());
      continue;
    }

    // Create a node for the control mode
    const auto options = rclcpp::NodeOptions(node_->get_node_options())
      .context(node_->get_node_base_interface()->get_context());
    const auto node_name = input_source_name;
    const auto input_source_node = std::make_shared<rclcpp::Node>(node_name, node_->get_namespace(), options);

    // Initialize the control mode
    input_source_class->initialize(input_source_node, input_source_name);

    RCLCPP_INFO(node_->get_logger(), "Registering input source \"%s\" as type \"%s\"...", input_source_name.c_str(),
      input_source_type.c_str());
    sources_.emplace_back(input_source_class);
  }
}

bool InputSourceManager::get_type_for_input_source(const std::string& name, std::string& source_type) const {
  // TODO: Check that the parameter hasn't already been defined
  node_->declare_parameter(name + ".type", rclcpp::ParameterType::PARAMETER_STRING);
  // TODO: Remember that this parameter has already been defined

  rclcpp::Parameter param;
  const auto result = node_->get_parameter(name + ".type", param);

  if (result)
    source_type = param.as_string();
  return result;
}

} // namespace teleop_arm_joy