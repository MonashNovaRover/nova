/**
 * @file teleop_arm_joy.cpp
 * @brief Teleop Arm Joy node to translate Joy messages from /joy to commands for the arm.
 * Edited by Bailey
 */

#include "teleop_arm_joy/teleop_arm_joy.hpp"

using namespace std::chrono_literals;

namespace
{
  constexpr auto BUTTON_DEBOUNCE_INTERVAL = std::chrono::milliseconds(50);
  constexpr auto DEFAULT_FK_VELOCITY_TOPIC = "/arm_fk_velocity_target";
}

using std::placeholders::_1;

namespace teleop_arm_joy
{

TeleopArmJoy::TeleopArmJoy(const rclcpp::NodeOptions &options)
    : Node("teleop_arm_joy_node", options)
{
  // Create publishers

  // Create subscribers
  fk_velocity_pub = this->create_publisher<nova_interfaces::msg::ArmFkVelocityTargets>(
    DEFAULT_FK_VELOCITY_TOPIC, 50);

  // Create service clients
  switch_controller_client = this->create_client<controller_manager_msgs::srv::SwitchController>("/controller_manager/switch_controller");
  // TODO: Make good for actual controller implementations
  fk_client = this->create_client<rcl_interfaces::srv::SetParameters>("/pivot_drive_controller/set_parameters");
  ik_client = this->create_client<rcl_interfaces::srv::SetParameters>("/strafe_controller/set_parameters");

  control_mode = ControlMode::FK;

  devices = std::vector<shared_ptr<JoyDevice>>();
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

  // Validate that there is at least one device
  if (params_.device_names.empty()) {
    RCLCPP_ERROR(this->get_logger(), "No device names defined!");
    return;
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
  for (auto& [button_name, button_config] : params_.buttons.button_definitions_map) {
    // Buttons without a definition will have their value be -1 by default, so we can filter them out.
    if (button_config.id < 0)
      continue;

    shared_ptr<JoyButton> button(new JoyButton(button_config));
    buttons[button_name] = button;
    listeners[button_config.device]->emplace_back(button);
  }

  for (auto& [axis_name, axis_config] : params_.axes.axis_definitions_map) {
    // Axes without a definition will have their value be -1 by default, so we can filter them out.
    if (axis_config.id < 0)
      continue;

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
  sendArmCommand();
}

void TeleopArmJoy::updateState() {
  previous_state = current_state;

  // Lock and unlock
  if (current_state.locked) {
    if (buttons["lock"]->down())
      current_state.locked = true;
  }
  else {
    if (buttons["unlock"]->down())
      current_state.locked = true;
  }

  // TODO: put speed into state
  handleSpeedChange();
}

// TODO: Implement strategy pattern for sendArmCommand and sendHaltCommand

void TeleopArmJoy::sendArmCommand()
{
  if (current_state.locked)
    return;

  auto msg = std::make_unique<nova_interfaces::msg::ArmFkVelocityTargets>();

  msg->header.stamp = this->now();

  for (auto [joint_name, joint_config] : params_.joints.joint_definitions_map) {
    msg->name.emplace_back(joint_name);

    const float input = axes[joint_name]->value();
    double velocity = static_cast<double>(input) * speed * joint_config.max_speed;

    msg->velocity.emplace_back(velocity);
  }

  fk_velocity_pub->publish(std::move(msg));
}


void TeleopArmJoy::sendHaltCommand()
{
  // Send all zeroes
  auto msg = std::make_unique<nova_interfaces::msg::ArmFkVelocityTargets>();

  msg->header.stamp = this->now();

  for (auto [joint_name, joint_config] : params_.joints.joint_definitions_map) {
    msg->name.emplace_back(joint_name);
    msg->velocity.emplace_back(0.0);
  }

  fk_velocity_pub->publish(std::move(msg));
}

void TeleopArmJoy::handleSpeedChange() {
  speed = 0.5f * (axes["speed"]->value() + 1.f);
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