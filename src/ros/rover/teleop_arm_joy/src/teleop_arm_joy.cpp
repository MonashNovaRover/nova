/**
 * @file teleop_arm_joy.cpp
 * @brief Teleop Arm Joy node to translate Joy messages from /joy to commands for the arm.
 * Edited by Abby
 */

#include "teleop_arm_joy/teleop_arm_joy.hpp"

using namespace std::chrono_literals;

namespace
{
  constexpr auto BUTTON_DEBOUNCE_INTERVAL = std::chrono::milliseconds(50);
  constexpr auto DEFAULT_FK_VELOCITY_TOPIC = "/arm_fk_velocity_target";
  constexpr auto DEFAULT_IK_TWIST_TOPIC = "/arm_ik_twist_stamped";
  constexpr auto DEFAULT_AUTO_TYPING_TOPIC = "/test";
}

using std::placeholders::_1;
using std::placeholders::_2;

namespace teleop_arm_joy
{

TeleopArmJoy::TeleopArmJoy(const rclcpp::NodeOptions &options)
    : Node("teleop_arm_joy_node", options)
{
  // Create publishers

  // Create subscribers
  fk_velocity_pub = this->create_publisher<nova_interfaces::msg::ArmFkVelocityTargets>(
    DEFAULT_FK_VELOCITY_TOPIC, 50);

  ik_twist_pub = this->create_publisher<geometry_msgs::msg::TwistStamped>(
    DEFAULT_IK_TWIST_TOPIC, 50);

  // Create service clients
  switch_controller_client = this->create_client<controller_manager_msgs::srv::SwitchController>("/controller_manager/switch_controller");
  // TODO: Make good for actual controller implementations
  fk_client = this->create_client<rcl_interfaces::srv::SetParameters>("/nova_arm_controller/set_parameters");
  ik_client = this->create_client<rcl_interfaces::srv::SetParameters>("/strafe_controller/set_parameters");
  
  service = this->create_service<std_srvs::srv::Trigger>("/teleop_arm_joy/toggle_typing", 
		  std::bind(&teleop_arm_joy::TeleopArmJoy::toggleTyping, this, _1, _2));
  
  control_mode = ControlMode::FK;
  typing_active = false;

  devices = std::vector<shared_ptr<JoyDevice>>();
  speed = 0;
}

void TeleopArmJoy::initializeParams()
{
  param_listener_ = std::make_shared<ParamListener>(this->shared_from_this());
  this->param

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

  // Validate that there is at least one device
  if (params_.device_names.empty()) {
    RCLCPP_ERROR(this->get_logger(), "No device names defined!");
    return;
  }

  // Create sinks for axes and buttons that don't get defined in the parameter file.
  shared_ptr<JoyAxis> sink_axis(new JoyAxis(Params::Axes::MapAxisDefinitions()));
  for (auto& axis_name : params_.axis_definitions) {
    axes[axis_name] = sink_axis;
  }
  shared_ptr<JoyButton> sink_button(new JoyButton(Params::Buttons::MapButtonDefinitions()));
  for (auto& button_name : params_.button_definitions) {
    buttons[button_name] = sink_button;
  }

  // Create device instances
  auto device_configs = params_.devices.device_names_map;
  devices.reserve(device_configs.size());

  // Create listener collections for each device, that we can add buttons and axes to later create JoyDevices from.
  auto listeners = map<string, vector<shared_ptr<JoyMessageListener>>*>();
  for (auto& name : params_.device_names) {
    listeners[name] = new vector<shared_ptr<JoyMessageListener>>();
  }
  listeners[""] = listeners[params_.device_names[0]];

  // Create button and axis objects
  RCLCPP_INFO(this->get_logger(), "Registered Buttons:");
  for (auto& [button_name, button_config] : params_.buttons.button_definitions_map) {
    // Buttons without a definition will have their value be -1 by default, so we can filter them out.
    if (button_config.id < 0)
      continue;

    RCLCPP_INFO(this->get_logger(), "  %s", button_name.c_str());

    shared_ptr<JoyButton> button(new JoyButton(button_config));
    buttons[button_name] = button;
    listeners[button_config.device]->emplace_back(button);
  }

  RCLCPP_INFO(this->get_logger(), "Registered Axes:");
  for (auto& [axis_name, axis_config] : params_.axes.axis_definitions_map) {
    // Axes without a definition will have their value be -1 by default, so we can filter them out.
    if (axis_config.id < 0 && axis_config.button_id_negative < 0 && axis_config.button_id_positive < 0)
      continue;

    RCLCPP_INFO(this->get_logger(), "  %s", axis_name.c_str());

    shared_ptr<JoyAxis> axis(new JoyAxis(axis_config));
    axes[axis_name] = axis;
    listeners[axis_config.device]->emplace_back(axis);
  }

  // Populate unspecified buttons and axes with duds
  shared_ptr<JoyButton> default_button(new JoyButton(Params::Buttons::MapButtonDefinitions()));
  for (auto& name : params_.button_definitions) {
    buttons.insert(make_pair(name, default_button));
  }

  shared_ptr<JoyAxis> default_axis(new JoyAxis(Params::Axes::MapAxisDefinitions()));
  for (auto& name : params_.button_definitions) {
    axes.insert(make_pair(name, default_axis));
  }

  // Give axes and buttons to a joy device to be managed
  for (auto& [name, config] : device_configs) {
    shared_ptr<JoyDevice> device(new JoyDevice(this, name, config, *listeners[name], bind(&TeleopArmJoy::onDeviceUpdated, this, _1)));
    devices.emplace_back(device);

    // Clean up
    delete listeners[name];
  }
  listeners.clear();

  RCLCPP_INFO(this->get_logger(), "Finished initializing params");
}

void TeleopArmJoy::onDeviceUpdated(string &device_name) {
  for (auto& device : devices) {
    device->debounce();
  }

  // Log any button presses
  for (auto& [name, button] : buttons) {
    if (button->down()) {
      RCLCPP_INFO(this->get_logger(), "  > %s pressed", name.c_str());
    }
  }

  // Log any button presses
  for (auto& [name, axis] : axes) {
    if (axis->changed()) {
      RCLCPP_INFO(this->get_logger(), "  > %s : %f", name.c_str(), axis->value());
    }
  }

  // Do actual stuff
  updateState();

  if (current_state.locked) {
    sendHaltCommand();
  }
  else {
    sendArmCommand();
  }
}

void TeleopArmJoy::updateState() {
  previous_state = current_state;

  // Lock and unlock
  if (!current_state.locked) {
    if (buttons["lock"]->down()) {
      current_state.locked = true;
      RCLCPP_INFO(this->get_logger(), "LOCKED");
    }
  }
  else {
    if (buttons["unlock"]->down()) {
      current_state.locked = false;
      RCLCPP_INFO(this->get_logger(), "UNLOCKED");
    }
  }

  // TODO: put speed into state
  handleSpeedChange();

  if (!typing_active)
  {
  	setControlMode(buttons["twist_mode"]->value() ? ControlMode::IK : ControlMode::FK);
  }
}

void TeleopArmJoy::setControlMode(const ControlMode new_control_mode) {
  if (control_mode == new_control_mode)
    return;

  sendHaltCommand();
  switchController(new_control_mode);

  control_mode = new_control_mode;

  if (new_control_mode == ControlMode::FK) {
    RCLCPP_INFO(get_logger(), "Switched to FK control.");
  }
  else if (new_control_mode == ControlMode::IK) {
    RCLCPP_INFO(get_logger(), "Switched to IK control.");
  }
  else if (new_control_mode == ControlMode::PathPlanner) {
    RCLCPP_INFO(get_logger(), "Switched to Path Planner control.");
  }
}

void TeleopArmJoy::sendArmCommand()
{
  if (current_state.locked)
    return;

  if (control_mode == ControlMode::FK) {
    sendJointSpaceCommand();
  }
  else if (control_mode == ControlMode::IK) {
    sendTwistCommand();
  }
  else if (control_mode == ControlMode::PathPlanner) {
    sendTwistCommand();
  }
}

void TeleopArmJoy::sendJointSpaceCommand()
{
  auto msg = std::make_unique<nova_interfaces::msg::ArmFkVelocityTargets>();

  msg->header.stamp = this->now();

  for (const auto& [joint_name, joint_config] : params_.joints.joint_definitions_map) {
    msg->name.emplace_back(joint_name);

    if (!axes.count(joint_name)) {
      RCLCPP_WARN(this->get_logger(), "Axis for joint with name '%s' does not exist!", joint_name.c_str());
      msg->velocity.emplace_back(0);
      continue;
    }

    const float input = axes[joint_name]->value();
    double velocity = static_cast<double>(input) * speed * joint_config.max_speed;

    msg->velocity.emplace_back(velocity);
  }

  fk_velocity_pub->publish(std::move(msg));

  auto ik_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();

  ik_msg->header.stamp = this->now();

  auto linear = geometry_msgs::msg::Vector3();
  linear.x = linear.y = linear.z = 0;

  auto angular = geometry_msgs::msg::Vector3();
  angular.x = angular.y = angular.z = 0;

  ik_msg->twist.linear = linear;
  ik_msg->twist.angular = angular;

  ik_twist_pub->publish(std::move(ik_msg));
}

void TeleopArmJoy::sendTwistCommand() {
  auto msg = std::make_unique<geometry_msgs::msg::TwistStamped>();

  msg->header.stamp = this->now();

  const auto linear_speed = speed * params_.control_modes.twist.linear_max;
  auto linear = geometry_msgs::msg::Vector3();
  linear.x = axes["twist_x"]->value() * linear_speed;
  linear.y = axes["twist_y"]->value() * linear_speed;
  linear.z = axes["twist_z"]->value() * linear_speed;

  const auto angular_speed = speed * params_.control_modes.twist.angular_max;
  auto angular = geometry_msgs::msg::Vector3();
  angular.x = axes["twist_roll" ]->value() * angular_speed;
  angular.y = axes["twist_pitch"]->value() * angular_speed;
  angular.z = axes["twist_yaw"  ]->value() * angular_speed;

  msg->twist.linear = linear;
  msg->twist.angular = angular;

  ik_twist_pub->publish(std::move(msg));
}

void TeleopArmJoy::sendHaltCommand()
{
  // Send all zeroes for joint space
  auto msg = std::make_unique<nova_interfaces::msg::ArmFkVelocityTargets>();

  msg->header.stamp = this->now();

  for (const auto& [joint_name, joint_config] : params_.joints.joint_definitions_map) {
    msg->name.emplace_back(joint_name);
    msg->velocity.emplace_back(0.0);
  }

  fk_velocity_pub->publish(std::move(msg));

  // Send halt command for IK
  auto ik_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();

  ik_msg->header.stamp = this->now();

  auto linear = geometry_msgs::msg::Vector3();
  linear.x = linear.y = linear.z = 0;

  auto angular = geometry_msgs::msg::Vector3();
  angular.x = angular.y = angular.z = 0;

  ik_msg->twist.linear = linear;
  ik_msg->twist.angular = angular;

  ik_twist_pub->publish(std::move(ik_msg));

  //TODO: halt auto typing command
}

void TeleopArmJoy::handleSpeedChange() {
  speed = 0.5f * (axes["speed"]->value() + 1.f);
}

std::vector<std::string> TeleopArmJoy::modeToControllers(const ControlMode mode) {
  switch (mode)
  {
    case ControlMode::FK:
      return params_.control_modes.joint_space.controllers;
    case ControlMode::IK:
      return params_.control_modes.twist.controllers;
    case ControlMode::PathPlanner:
      return params_.control_modes.path_planner_ik.controllers;
    default:
      RCLCPP_WARN(get_logger(), "Unknown control type given to modeToControllers. Returning no controllers.");
      return {};
  }
}

void TeleopArmJoy::switchController(const ControlMode requested_control_mode)
{
  if (requested_control_mode == control_mode)
    return;

  RCLCPP_INFO(this->get_logger(), "Changing from %s to %s",
              prettyPrintMode(control_mode).c_str(),
              prettyPrintMode(requested_control_mode).c_str());

  // The order of deactivation needs to be opposite to the activation order. This is the reverse to the final order.
  vector<std::string> deactivate_controllers_reversed = modeToControllers(control_mode);
  // Reverse the given order of controllers so they deactivate correctly; in order.
  vector<std::string> deactivate_controllers(deactivate_controllers_reversed.size());
  std::reverse_copy(deactivate_controllers_reversed.begin(), deactivate_controllers_reversed.end(),
                    deactivate_controllers.begin());

  // Given order of activated controllers is already correct
  vector<std::string> activate_controllers = modeToControllers(requested_control_mode);

  if (!switch_controller_client->service_is_ready()) {
    RCLCPP_ERROR(this->get_logger(), "Controller manager service not available.");
    return;
  }

  auto request = std::make_shared<controller_manager_msgs::srv::SwitchController::Request>();
  request->deactivate_controllers = deactivate_controllers;
  request->activate_controllers = activate_controllers;
  request->strictness = 2;
  request->activate_asap = true;

  auto future = switch_controller_client->async_send_request(request);
  control_mode = requested_control_mode;
}

bool TeleopArmJoy::setTypingState()
{
  if (!typing_active)
  {
  	// TODO: error checking!! right now this assumes that the control mode switch always goes through
  	setControlMode(ControlMode::PathPlanner);
  }

  typing_active = !typing_active;
  
  return true;
}

void TeleopArmJoy::toggleTyping(const std::shared_ptr<std_srvs::srv::Trigger::Request> request, std::shared_ptr<std_srvs::srv::Trigger::Response> response)
{
  RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Received request to toggle typing state.");

  bool res = setTypingState();
  if (!res) {
	response->success = false;
    response->message = "Could not switch to typing mode.";
  }
  else {
  	response->success = true;
	response->message = typing_active ? "Switched to typing mode." : "Switched to FK/IK mode.";
  }

  RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "Sending back response...");
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
