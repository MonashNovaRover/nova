//
// Created by nova on 6/11/25.
//

#include "teleop_arm_joy/input_sources/InputSourceManager.hpp"

#include "colors.h"
#include "utils.hpp"

namespace teleop_arm_joy {

void InputSourceManager::configure(const std::shared_ptr<ParamListener>& param_listener, InputManager& inputs) {
  const auto logger = node_->get_logger();

  param_listener_ = param_listener;
  params_ = param_listener->get_params();

  setup_input_sources();
}

bool InputSourceManager::get_type_for_input_source(const std::string& name, std::string& source_type) const {
  // TODO: Check that the parameter hasn't already been defined
  node_->declare_parameter("input_sources." + name + ".type", rclcpp::ParameterType::PARAMETER_STRING);
  // TODO: Remember that this parameter has already been defined

  rclcpp::Parameter param;
  const auto result = node_->get_parameter("input_sources." + name + ".type", param);

  if (result)
    source_type = param.as_string();
  return result;
}

// Runs on input source threads
void InputSourceManager::on_input_source_requested_update(const rclcpp::Time& now) {
  // TOOD: Consider out of order timestamps
  {
    std::lock_guard lock(mutex_);
    should_update_ = true;
    update_time_ = now;
  }

  update_condition_.notify_one();
}

rclcpp::Time InputSourceManager::wait_for_update() {
  if (params_.min_update_rate > 0) {
    const std::chrono::duration<double> min_wait_period{1.0 / params_.min_update_rate};
    std::unique_lock lock(mutex_);
    update_condition_.wait_for(lock, min_wait_period, [&]{ return should_update_.load(); });
  }
  else {
    std::unique_lock lock(mutex_);
    update_condition_.wait(lock, [&]{ return should_update_.load(); });
  }

  should_update_ = false;
  return update_time_;
}

void InputSourceManager::update(const rclcpp::Time& now) const {
  for (const auto& source : sources_) {
    source.source->update(now);
  }
}

InputSourceManager::InputSourceHandle InputSourceManager::create_input_source_handle(
  const std::shared_ptr<InputSource>& input_source_class) const {
  InputSourceHandle handle{input_source_class};

  // Export input declarations
  std::vector<InputDeclaration<bool>> button_declarations{};
  input_source_class->_export_buttons(button_declarations);

  std::vector<InputDeclaration<double>> axis_declarations{};
  input_source_class->_export_axes(axis_declarations);

  // Create input declaration handles for all the input declarations
  handle.button_handles.reserve(button_declarations.size());
  for (const auto& declaration : button_declarations) {
    InputDeclarationHandle<bool> button_handle{declaration};
    handle.button_handles.emplace_back(button_handle);
  }

  handle.axis_handles.reserve(axis_declarations.size());
  for (const auto& declaration : axis_declarations) {
    InputDeclarationHandle<double> axis_handle{declaration};
    handle.axis_handles.emplace_back(axis_handle);
  }

  return handle;
}

void InputSourceManager::add_input_source_input_definitions(InputSourceHandle& handle) const {
  // Export to inputs
  auto& inputs = inputs_.get();

  for (const auto& [declaration] : handle.axis_handles)
    inputs.get_axes()[declaration.name]->add_definition(declaration.reference);

  for (const auto& [declaration] : handle.button_handles)
    inputs.get_buttons()[declaration.name]->add_definition(declaration.reference);
}

void InputSourceManager::remove_input_source_input_definitions(InputSourceHandle& handle) const {
  // Unexport to inputs
  auto& inputs = inputs_.get();

  for (const auto& [declaration] : handle.axis_handles)
    inputs.get_axes()[declaration.name]->remove_definition(declaration.reference);

  for (const auto& [declaration] : handle.button_handles)
    inputs.get_buttons()[declaration.name]->remove_definition(declaration.reference);
}

bool InputSourceManager::create_input_source(const std::string& input_source_name) {
  const auto logger = node_->get_logger();

  std::string input_source_type;
  const std::string pretty_name = snake_to_title(input_source_name);

  // Get the control mode's plugin type name
  if (!get_type_for_input_source(input_source_name, input_source_type)) {
    RCLCPP_ERROR(logger, "Failed to find type for input source \"%s\" in params. Have you defined %s.type in your "
                 "parameter file?", input_source_name.c_str(), input_source_name.c_str());
    // registered_sources_log << C_FAIL_QUIET << "\n\t- " << pretty_name << C_FAIL_QUIET << "\t(failed - " << input_source_name << ".type param missing) " << C_RESET;
    return false;
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
    // registered_sources_log << C_FAIL_QUIET << "\n\t- " << pretty_name << C_FAIL_QUIET << "\t(failed - can't find plugin " << input_source_type << ") " << C_RESET;
    return false;
  }

  // Create a node for the control mode
  const auto options = rclcpp::NodeOptions(node_->get_node_options())
      .context(node_->get_node_base_interface()->get_context());
  const auto& node_name = input_source_name;
  const auto input_source_node = std::make_shared<rclcpp::Node>(node_name, node_->get_namespace(), options);

  // Initialize the control mode
  input_source_class->initialize(input_source_node, input_source_name, shared_from_this());
  auto handle = create_input_source_handle(input_source_class);

  // Export the inputs
  add_input_source_input_definitions(handle);

  if (const auto executor = executor_.lock()) {
    executor->add_node(input_source_class->get_node());
  }

  return true;
}

void InputSourceManager::setup_input_sources() {
  const auto logger = node_->get_logger();
  const auto& inputs = inputs_.get();

  // Declare and get parameter for control modes to spawn by default
  node_->declare_parameter("input_sources.names", rclcpp::ParameterType::PARAMETER_STRING_ARRAY);
  rclcpp::Parameter input_sources_param;
  node_->get_parameter("input_sources.names", input_sources_param);

  // Pluginlib for loading control modes dynamically
  source_loader_ = std::make_unique<pluginlib::ClassLoader<InputSource>>(
    "teleop_arm_joy", "teleop_arm_joy::InputSource");

  // List available input source plugins
  try
  {
    std::stringstream available_plugins_log{};
    const auto plugins = source_loader_->getDeclaredClasses();
    for (const auto& plugin : plugins) {
      available_plugins_log << "\n\t- " << plugin;
    }

    RCLCPP_DEBUG(logger, "Registered InputSource plugins:%s", available_plugins_log.str().c_str());
  }
  catch (const pluginlib::PluginlibException& ex)
  {
    RCLCPP_ERROR(logger, "Failed to list input source plugins! what(): %s", ex.what());
    return;
  }

  // Create each control mode according to the given params
  const auto input_source_names = input_sources_param.get_type() == rclcpp::PARAMETER_STRING_ARRAY ?
     input_sources_param.as_string_array() : std::vector<std::string>();

  std::stringstream registered_sources_log{};
  for (auto& input_source_name : input_source_names) {
    create_input_source(input_source_name);
  }

  RCLCPP_INFO(logger, C_TITLE "Input Sources:" C_RESET "%s\n", registered_sources_log.str().c_str());



}

} // namespace teleop_arm_joy