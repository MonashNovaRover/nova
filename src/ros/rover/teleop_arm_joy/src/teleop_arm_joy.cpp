/**
 * @file teleop_arm_joy.cpp
 * @brief Teleop Arm Joy node to translate Joy messages from /joy to commands for the arm.
 * Edited by Bailey
 */

#include "teleop_arm_joy/teleop_arm_joy.hpp"

using namespace std::chrono_literals;

namespace
{
  constexpr auto DEFAULT_INPUT_TOPIC = "/joy";
  constexpr auto DEFAULT_OUTPUT_TOPIC = "/"; // TODO
  constexpr auto DEFAULT_OUTPUT_TOPIC_INFO = "/"; // TODO
  constexpr auto BUTTON_DEBOUNCE_INTERVAL = std::chrono::milliseconds(100);
}

using std::placeholders::_1;

namespace teleop_arm_joy
{

TeleopArmJoy::TeleopArmJoy(const rclcpp::NodeOptions &options)
    : Node("teleop_arm_joy_node", options)
{
  // Create publishers

  // Create subscribers
  //    joy_sub = this->create_subscription<sensor_msgs::msg::Joy>(
  //        DEFAULT_INPUT_TOPIC, rclcpp::QoS(10), std::bind(&TeleopArmJoy::joyCallback, this, _1));

  // Create service clients
  switch_controller_client = this->create_client<controller_manager_msgs::srv::SwitchController>("/controller_manager/switch_controller");
  // TODO: Make good for actual controller implementations
  fk_client = this->create_client<rcl_interfaces::srv::SetParameters>("/pivot_drive_controller/set_parameters");
  ik_client = this->create_client<rcl_interfaces::srv::SetParameters>("/strafe_controller/set_parameters");

  control_mode = ControlMode::FK;

  devices = std::vector<JoyDevice>();
  speed = 0;
}

void TeleopArmJoy::initializeParams()
{
  param_listener_ = std::make_shared<ParamListener>(this->shared_from_this());
  params_ = param_listener_->get_params();

  if (param_listener_->is_old(params_))
  {
    params_ = param_listener_->get_params();
  }

  // Tidy up any existing elements
  // TODO: Check this actually tidies up the existing elements
  devices.clear();
  buttons.clear();
  axes.clear();

  // Create device instances
  auto device_configs = params_.device_mappings.devices_map;
  devices.reserve(device_configs.size());

  // Loop over each device config and create a device for it
  for (auto& [name, config] : device_configs) {
    auto listeners = vector<shared_ptr<JoyMessageListener>>();
    listeners.reserve(config.buttons.button_definitions_map.size() + config.axes.axis_definitions_map.size());

    // Create button and axis objects
    for (auto& [button_name, button_config] : config.buttons.button_definitions_map) {
      // Buttons without a definition will have their value be -1 by default, so we can filter them out.
      if (button_config.id < 0)
        continue;

      shared_ptr<JoyButton> button(new JoyButton(button_config));
      buttons[button_name] = button;
      listeners.emplace_back(button);
    }

    for (auto& [axis_name, axis_config] : config.axes.axis_definitions_map) {
      // Axes without a definition will have their value be -1 by default, so we can filter them out.
      if (axis_config.id < 0)
        continue;

      shared_ptr<JoyAxis> axis(new JoyAxis(axis_config));
      axes[axis_name] = axis;
      listeners.emplace_back(axis);
    }

    // Give axes and axes to a joy device to be managed
    auto device = JoyDevice(name, listeners);
    devices.emplace_back(device);
  }

}

void TeleopArmJoy::sendArmCommand(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
{
  // auto controller_params = params_.controllers_map.at(modeToController(control_mode));


}

void TeleopArmJoy::sendHaltCommand()
{
}

void TeleopArmJoy::handleButtonCallbacks(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
{
  auto now = this->now();

  auto isDebounced = [&](int buttonValue, int buttonIndex) -> bool
  {
    if (last_button_press_time_.find(buttonIndex) == last_button_press_time_.end() ||
        (now - last_button_press_time_[buttonIndex]) > rclcpp::Duration(BUTTON_DEBOUNCE_INTERVAL))
    {
      last_button_press_time_[buttonIndex] = now;
      return buttonValue;
    }
    return 0;
  };

  // Lock and Unlock

  // TODO: Controller switching when the respective button is pressed

  if (current_state.locked)
    return;

  // TODO: Speed change input
}

void TeleopArmJoy::switchController(const ControlMode requested_control_mode)
{
  if (requested_control_mode == control_mode)
    return;

  RCLCPP_INFO(this->get_logger(), "Changing from %s to %s",
              prettyPrintMode(control_mode).c_str(),
              prettyPrintMode(requested_control_mode).c_str());

  std::string deactivate_controller = modeToController(control_mode);
  std::string activate_controller = modeToController(requested_control_mode);

  if (!switch_controller_client->service_is_ready()) {
      RCLCPP_ERROR(this->get_logger(), "Controller manager service not available.");
      return;
  }

  auto request = std::make_shared<controller_manager_msgs::srv::SwitchController::Request>();
  request->activate_controllers.emplace_back(activate_controller);
  request->deactivate_controllers.emplace_back(deactivate_controller);
  request->strictness = 2;
  request->activate_asap = true;

  auto future = switch_controller_client->async_send_request(request);

  control_mode = requested_control_mode;
}


}

int main(int argc, char *argv[])
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<teleop_arm_joy::TeleopArmJoy>();
  node->initializeParams();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}