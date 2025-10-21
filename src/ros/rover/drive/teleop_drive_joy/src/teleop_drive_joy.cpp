/**
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Monash Nova Rover Team
 *
 * PACKAGE: teleop_drive_joy
 * AUTHORS:	Kabi, Terry Tian
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

// Define all colors
#define C_END "\033[0m"
#define C_FAIL "\033[1;31m"
#define C_SUCCESS "\033[1;32m"
#define C_MODE "\033[;33m"
#define C_INFO "\033[1;34m"
#define C_SPEED "\033[;35m"
#define C_TITLE "\033[1;36m"

#include <utility>
#include <tuple>
#include <algorithm>

#include "teleop_drive_joy/teleop_drive_joy.hpp"

using namespace std::chrono_literals;

using std::placeholders::_1;

namespace teleop_drive_joy
{

TeleopDriveJoy::TeleopDriveJoy(const rclcpp::NodeOptions& options)
  : Node("teleop_drive_joy_node", options)
  , sent_lock_msg_(false)
  , locked_(true)
  , drive_mode_(DriveMode::PIVOT)
{
}

std::pair<std::pair<double, double>, double> TeleopDriveJoy::snapped_joy_axes(
  const sensor_msgs::msg::Joy::SharedPtr joy_msg)
{
  double linear_x = joy_msg->axes[params_.axis_linear_x];
  double linear_y = joy_msg->axes[params_.axis_linear_y];
  double angular_z = joy_msg->axes[params_.axis_angular_z];

  // Apply max input thresholds
  double linear_magnitude =
    std::hypot(joy_msg->axes[params_.axis_linear_x], joy_msg->axes[params_.axis_linear_y]);
  if (linear_magnitude > params_.max_input_threshold)
  {
    // Normalise the axes for max input
    linear_x /= linear_magnitude;
    linear_y /= linear_magnitude;
  }

  double angular_magnitude =
    std::hypot(joy_msg->axes[params_.axis_angular_z], joy_msg->axes[params_.axis_linear_z]);
  if (angular_magnitude > params_.max_input_threshold)
  {
    angular_z /= angular_magnitude;
  }

  return {{linear_x, linear_y}, angular_z};
}

void TeleopDriveJoy::set_autonomous_mode_for_controllers(bool enable)
{
  // Create the parameter request
  auto request = std::make_shared<rcl_interfaces::srv::SetParameters::Request>();
  // Construct the parameter manually
  rcl_interfaces::msg::Parameter param;
  param.name = "autonomous_mode";
  param.value.type = rcl_interfaces::msg::ParameterType::PARAMETER_BOOL;
  param.value.bool_value = enable;
  // Add the parameter to the request
  request->parameters.push_back(param);

  for (const auto& client : {pivot_drive_client_, strafe_client_, diff_drive_client_})
  {
    if (!client->service_is_ready())
    {
      RCLCPP_ERROR(
        this->get_logger(), "%s service is not ready for client.", client->get_service_name());
      continue;
    }

    static_cast<void>(client->async_send_request(request));
  }

}  // namespace teleop_drive_joy

void TeleopDriveJoy::initialize()
{
  initialize_params();
  initialize_interfaces();
  map_button_callbacks();
  print_controls();
  RCLCPP_INFO_STREAM(
    this->get_logger(), C_INFO << "Initialized with control mode: " << C_MODE
                               << pretty_print_mode(drive_mode_) << C_END ", " << C_SPEED
                               << "speed: " << std::to_string(speed_) << C_END);
  RCLCPP_INFO_STREAM(
    this->get_logger(), C_FAIL << "Gamepad is locked. Press the unlock button to unlock." << C_END);
}

void TeleopDriveJoy::initialize_params()
{
  param_listener_ = std::make_shared<ParamListener>(this->shared_from_this());
  params_ = param_listener_->get_params();

  if (param_listener_->is_old(params_))
  {
    params_ = param_listener_->get_params();
  }
  speed_ = params_.initial_speed;
}

void TeleopDriveJoy::initialize_interfaces()
{
  joy_sub_ = this->create_subscription<sensor_msgs::msg::Joy>(
    params_.input_topic, rclcpp::QoS(10), std::bind(&TeleopDriveJoy::joy_callback, this, _1));
  cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::TwistStamped>(params_.output_topic, 50);

  switch_controller_client_ = this->create_client<controller_manager_msgs::srv::SwitchController>(
    "/controller_manager/switch_controller");
  pivot_drive_client_ = this->create_client<rcl_interfaces::srv::SetParameters>(
    "/pivot_drive_controller/set_parameters");
  strafe_client_ = this->create_client<rcl_interfaces::srv::SetParameters>(
    "/strafe_drive_controller/set_parameters");
  diff_drive_client_ = this->create_client<rcl_interfaces::srv::SetParameters>(
    "/diff_drive_controller/set_parameters");
}

void TeleopDriveJoy::map_button_callbacks()
{
  button_callbacks_[params_.button_unlock] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (locked_)
    {
      locked_ = false;
      RCLCPP_INFO_STREAM(this->get_logger(), C_SUCCESS << "Gamepad unlocked" << C_END);
    }
  };
  button_callbacks_[params_.button_lock] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (!locked_)
    {
      locked_ = true;
      RCLCPP_INFO_STREAM(this->get_logger(), C_FAIL << "Gamepad locked" << C_END);
    }
  };
  button_callbacks_[params_.button_autonomous_mode] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    set_autonomous_mode_for_controllers(true);
    RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << "Autonomous mode" << C_END);
  };
  button_callbacks_[params_.button_manual_mode] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    set_autonomous_mode_for_controllers(false);
    RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << "Manual mode" << C_END);
  };
  button_callbacks_[params_.button_pivot_drive_controller] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    switch_controller(DriveMode::PIVOT);
  };
  button_callbacks_[params_.button_holonomic_drive_controller] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    switch_controller(DriveMode::HOLONOMIC);
  };
  button_callbacks_[params_.button_strafe_drive_controller] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    switch_controller(DriveMode::STRAFE);
  };
  button_callbacks_[params_.button_diff_drive_controller] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    switch_controller(DriveMode::DIFF);
  };

  auto changeSpeed = [this](int speed_change)
  {
    if (!locked_)
    {
      speed_ = std::clamp(
        speed_ + speed_change,
        params_.speed_limit_min, params_.speed_limit_max);
      RCLCPP_INFO_STREAM(this->get_logger(), C_SPEED << "Speed: " << speed_ << C_END);
    }
  };
  button_callbacks_[params_.button_speed_decrease_fine] =
    [this, changeSpeed](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    changeSpeed(-params_.speed_change_fine_val);
  };
  button_callbacks_[params_.button_speed_increase_fine] =
    [this, changeSpeed](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    changeSpeed(params_.speed_change_fine_val);
  };
  button_callbacks_[params_.button_speed_decrease_coarse] =
    [this, changeSpeed](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    changeSpeed(-params_.speed_change_coarse_val);
  };
  button_callbacks_[params_.button_speed_increase_coarse] =
    [this, changeSpeed](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    changeSpeed(params_.speed_change_coarse_val);
  };
  button_callbacks_[-(params_.axis_hold_position)] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    RCLCPP_ERROR_STREAM(this->get_logger(), C_FAIL << "Failed to hold position: not implemented" << C_END);
  };
  button_callbacks_[-(params_.axis_handbrake)] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    RCLCPP_ERROR_STREAM(this->get_logger(), C_FAIL << "Failed to activate handbrake: not implemented" << C_END);
  };
}

void TeleopDriveJoy::joy_callback(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
{
  handle_button_callbacks(joy_msg);

  if (!locked_)
  {
    send_drive_command(joy_msg);
  }
  else
  {
    send_halt_command();
  }
}

void TeleopDriveJoy::handle_button_callbacks(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
{
  auto now = this->now();

  auto isPressedAndDebounced = [this, &now, joy_msg](int button_index)
  {
    bool pressed = (button_index >= 0 && joy_msg->buttons[button_index]) ||
                   (button_index < 0 && joy_msg->axes[std::abs(button_index)] <= -params_.trigger_pressed_threshold);
    bool debounced = last_button_press_time_.find(button_index) == last_button_press_time_.end() ||
                     now - last_button_press_time_[button_index] >
                       rclcpp::Duration(std::chrono::milliseconds(params_.button_debounce_time));
    return pressed && debounced;
  };

  for (const auto& [button_index, button_callback] : button_callbacks_)
  {
    if (isPressedAndDebounced(button_index))
    {
      last_button_press_time_[button_index] = now;
      button_callback(joy_msg);
    }
  }
}

void TeleopDriveJoy::send_drive_command(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
{
  const auto& controller_params = params_.controllers_map.at(mode_to_controller(drive_mode_));

  auto [linear, angular] = snapped_joy_axes(joy_msg);
  linear.first *= controller_params.scale_linear * speed_;
  linear.first =
    std::clamp(linear.first, -controller_params.limit_linear, controller_params.limit_linear);
  linear.second *= controller_params.scale_linear * speed_;
  linear.second =
    std::clamp(linear.second, -controller_params.limit_linear, controller_params.limit_linear);
  angular *= controller_params.scale_angular;
  angular = std::clamp(angular, -controller_params.limit_angular, controller_params.limit_angular);

  auto cmd_vel_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
  cmd_vel_msg->twist.linear.x = linear.first;
  cmd_vel_msg->twist.linear.y = linear.second;
  cmd_vel_msg->twist.angular.z = angular;
  cmd_vel_msg->header.stamp = this->now();

  cmd_vel_pub_->publish(std::move(cmd_vel_msg));

  sent_lock_msg_ = false;
}

void TeleopDriveJoy::send_halt_command()
{
  if (sent_lock_msg_) return;

  auto cmd_vel_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
  cmd_vel_pub_->publish(std::move(cmd_vel_msg));

  sent_lock_msg_ = true;
}

void TeleopDriveJoy::switch_controller(const DriveMode requested_control_mode)
{
  if (requested_control_mode == drive_mode_)
  {
    RCLCPP_INFO_STREAM(
      this->get_logger(),
      C_INFO << "Already in " << C_MODE << pretty_print_mode(drive_mode_) << C_END);
    return;
  }

  RCLCPP_INFO_STREAM(
    this->get_logger(), C_INFO << "Changing from " << C_MODE << pretty_print_mode(drive_mode_)
                               << C_INFO << " to " << C_MODE
                               << pretty_print_mode(requested_control_mode) << C_END);

  std::string deactivate_controller = mode_to_controller(drive_mode_);
  std::string activate_controller = mode_to_controller(requested_control_mode);

  if (!switch_controller_client_->service_is_ready())
  {
    RCLCPP_ERROR(this->get_logger(), "Controller manager service not available.");
    RCLCPP_ERROR_STREAM(
      this->get_logger(),
      C_FAIL << "Failed to switch to " << C_MODE << pretty_print_mode(requested_control_mode)
             << C_FAIL << ", remaining in " << C_MODE << pretty_print_mode(drive_mode_) << C_END);
    return;
  }

  auto request = std::make_shared<controller_manager_msgs::srv::SwitchController::Request>();
  request->activate_controllers.emplace_back(activate_controller);
  request->deactivate_controllers.emplace_back(deactivate_controller);
  request->strictness = 2;
  request->activate_asap = true;

  auto future = switch_controller_client_->async_send_request(request);

  drive_mode_ = requested_control_mode;
  RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << pretty_print_mode(drive_mode_) << C_END);
}

void TeleopDriveJoy::print_controls()
{
  // clang-format off
  RCLCPP_INFO_STREAM(this->get_logger(), C_TITLE << "Teleop Drive Joy Controls:" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "         Left Stick     |  Linear X/Y" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "        Right Stick     |  Angular Z" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "              Start     |  Unlock" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "               Back     |  Lock" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "       DPad Up/Down     |  Coarse Speed Change" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "    DPad Left/Right     |  Fine Speed Change" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "        Left Bumper     |  Autonomous Mode" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "       Right Bumper     |  Manual Mode" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "       Left Trigger     |  Hold Position (Holonomic Drive) (NOT YET IMPLEMENTED)" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "      Right Trigger     |  Handbrake (decrease speed by 40%) (NOT YET IMPLEMENTED)" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "           Button A     |  Pivot Drive" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "           Button B     |  Holonomic Drive (NOT YET IMPLEMENTED)" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "           Button X     |  Strafe Drive" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "           Button Y     |  Tank Drive" << C_END);
  // clang-format on
}

}  // namespace teleop_drive_joy

int main(int argc, char* argv[])
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<teleop_drive_joy::TeleopDriveJoy>();
  node->initialize();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}