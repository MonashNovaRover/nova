
#include "teleop_arm_joy/control_modes/ControlModeManager.hpp"

#include <controller_manager_msgs/srv/detail/switch_controller__struct.hpp>

namespace teleop_arm_joy {

void ControlModeManager::configure() {

  // Create service clients
  switch_controller_client_ = node_->create_client<controller_manager_msgs::srv::SwitchController>("/controller_manager/switch_controller");

  std::map<std::string, rclcpp::Parameter> params{};

  RCLCPP_INFO(node_->get_logger(), "Parameters for %s:", node_->get_name());

  // TODO: Test this parameter fetching logic!

  // Create control modes
  node_->get_parameters("", params);
  for (auto &param : params) {
    const auto name = param.first;
    RCLCPP_INFO(node_->get_logger(), "  - %s", name.c_str());

    // Make sure the parameter ends in "type"
    if (name.substr(name.size() - 4, name.size()) != "type")
      continue;

    auto control_mode_name = name.substr(name.size() - 5);
    RCLCPP_INFO(node_->get_logger(), "    (control mode \"%s\")", control_mode_name.c_str());

    const auto control_mode_type = param.second.as_string();
    RCLCPP_INFO(node_->get_logger(), "    (type \"%s\")", control_mode_type.c_str());
  }
}

bool ControlModeManager::set_control_mode(const std::string& name) {
  const auto new_control_mode_it = std::find(control_modes_.begin(), control_modes_.end(), name);

  // Ensure the given control mode exists
  if (new_control_mode_it == control_modes_.end() || !new_control_mode_it->second)
    return false;

  // Deactivate the previous control mode
  if (current_control_mode_) {
    current_control_mode_->deactivate();
  }
  const auto previous_control_mode_ = current_control_mode_;
  current_control_mode_ = nullptr;

  // Disable and enable controllers by calling controller manager
  auto switch_result = switch_controllers(*previous_control_mode_, *new_control_mode_it->second);

  if (!switch_result) {
    // TODO: Error recovery here
    return false;
  }

  // Activate the new control mode
  current_control_mode_ = new_control_mode_it->second;
  current_control_mode_->activate();

  return true;
}

void ControlModeManager::reset() {
  switch_controller_client_ = nullptr;
}

bool ControlModeManager::switch_controllers(const ControlMode& previous, const ControlMode& next) const {
  if (previous.get_name() == next.get_name())
    return false;

  RCLCPP_INFO(node_->get_logger(), "Changing from %s to %s",
              previous.get_name().c_str(),
              next.get_name().c_str());

  // The order of deactivation needs to be opposite to the activation order. This is the reverse to the final order.
  std::vector<std::string> deactivate_controllers_reversed = previous.get_base_params().controllers;
  // Reverse the given order of controllers so they deactivate correctly; in order.
  std::vector<std::string> deactivate_controllers(deactivate_controllers_reversed.size());
  std::reverse_copy(deactivate_controllers_reversed.begin(), deactivate_controllers_reversed.end(),
                    deactivate_controllers.begin());

  // Given order of activated controllers is already correct
  const std::vector<std::string> activate_controllers = next.get_base_params().controllers;

  if (!switch_controller_client_->service_is_ready()) {
    RCLCPP_ERROR(node_->get_logger(), "Controller manager service not available.");
    return false;
  }

  auto request = std::make_shared<controller_manager_msgs::srv::SwitchController::Request>();
  request->deactivate_controllers = deactivate_controllers;
  request->activate_controllers = activate_controllers;
  request->strictness = 2;
  request->activate_asap = true;

  auto future = switch_controller_client_->async_send_request(request);

  // TODO: Error recovery when the controller isn't able to switch the controllers.

  return true;
}

} // namespace teleop_arm_joy