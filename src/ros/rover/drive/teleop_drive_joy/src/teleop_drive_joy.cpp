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
  , handbrake_pressed_(false)
  , autonomous_mode_(false) // assume (maybe incorrectly) until set_autonomous_mode_for_controllers is called
  , connected_(false)
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

  autonomous_mode_ = enable;
  send_drive_info();

}  // namespace teleop_drive_joy

void TeleopDriveJoy::initialize()
{
  initialize_params();
  initialize_interfaces();
  map_button_callbacks();
  print_controls();

  // initialize connection timer
  connection_timer_ = this->create_timer(0.5s, [this]()
  {
    set_connected(false);
    connection_timer_->cancel();
  });

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

  drive_info_pub_ = this->create_publisher<drive_interfaces::msg::DriveInfo>(params_.drive_info_topic, 10);

  joint_state_sub_ = this->create_subscription<sensor_msgs::msg::JointState>(
    params_.joint_states_topic, rclcpp::QoS(10), std::bind(&TeleopDriveJoy::joint_states_callback, this, _1));
  joy_feedback_pub_ = this->create_publisher<sensor_msgs::msg::JoyFeedback>(params_.joy_feedback_topic, 10);
}

void TeleopDriveJoy::map_button_callbacks()
{
  button_callbacks_[params_.button_unlock] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (locked_)
    {
      locked_ = false;
      RCLCPP_INFO_STREAM(this->get_logger(), C_SUCCESS << "Gamepad unlocked" << C_END);
      send_drive_info();
    }
  };
  button_callbacks_[params_.button_lock] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (!locked_)
    {
      locked_ = true;
      RCLCPP_INFO_STREAM(this->get_logger(), C_FAIL << "Gamepad locked" << C_END);
      send_drive_info();
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
  button_callbacks_[params_.button_ackermann_drive_controller] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    switch_controller(DriveMode::ACKERMANN);
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

  auto change_speed = [this](double speed_change)
  {
    if (!locked_)
    {
      speed_ = std::clamp(
        speed_ + speed_change,
        params_.speed_limit_min, params_.speed_limit_max);
      RCLCPP_INFO_STREAM(this->get_logger(), C_SPEED << "Speed: " << speed_ << C_END);
      send_drive_info();
    }
  };
  button_callbacks_[params_.button_speed_decrease_fine] =
    [this, change_speed](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    change_speed(-params_.speed_change_fine_val);
  };
  button_callbacks_[params_.button_speed_increase_fine] =
    [this, change_speed](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    change_speed(params_.speed_change_fine_val);
  };
  button_callbacks_[params_.button_speed_decrease_coarse] =
    [this, change_speed](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    change_speed(-params_.speed_change_coarse_val);
  };
  button_callbacks_[params_.button_speed_increase_coarse] =
    [this, change_speed](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    change_speed(params_.speed_change_coarse_val);
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

  // update connection status
  connection_timer_->reset();
  set_connected(true);
}

void TeleopDriveJoy::handle_button_callbacks(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
{
  auto now = this->now();

  auto isPressedAndDebounced = [this, &now, joy_msg](int button_index)
  {
    bool pressed = joy_msg->buttons[button_index];
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

  // handbrake
  if (std::abs(joy_msg->axes[params_.axis_handbrake]) > params_.trigger_pressed_threshold)
  {
	if (!handbrake_pressed_)
	{
	  handbrake_pressed_ = true;
	  RCLCPP_INFO_STREAM(this->get_logger(), C_SUCCESS << "Handbrake activated" << C_END);
	  send_drive_info();
	}
    linear.first *= params_.handbrake_speed_multiplier;
    linear.second *= params_.handbrake_speed_multiplier;
  }
  else if (handbrake_pressed_)
  {
    handbrake_pressed_ = false;
    RCLCPP_INFO_STREAM(this->get_logger(), C_FAIL << "Handbrake deactivated" << C_END);
    send_drive_info();
  }

  // force zeros to be positive zero so ackermann drive assumes rover will move
  // forward when stopped (positioning pivots in the correct direction)
  if (linear.first == -0.0)
  {
    linear.first = +0.0;
  }

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

  send_drive_info();
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
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "      Right Trigger     |  Handbrake (decrease speed by 40%)" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "           Button A     |  Pivot Drive" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "           Button B     |  Ackermann Drive" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "           Button X     |  Strafe Drive" << C_END);
  RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "           Button Y     |  Tank Drive" << C_END);
  // clang-format on
}

void TeleopDriveJoy::set_connected(const bool connected)
{
    if (connected != connected_)
    {
      connected_ = connected;
      send_drive_info();

      if (connected_)
      {
        RCLCPP_ERROR_STREAM(this->get_logger(), C_INFO << "Gamepad " << C_SUCCESS << "connected." << C_END);
      }
      else
      {
        RCLCPP_ERROR_STREAM(this->get_logger(), C_INFO << "Gamepad " << C_FAIL << "disconnected." << C_END);
      }
    }
}

void TeleopDriveJoy::send_drive_info()
{
  drive_interfaces::msg::DriveInfo msg {};

  msg.autonomous_mode = autonomous_mode_;
  msg.connected = connected_;
  msg.drive_mode = mode_to_drive_info(drive_mode_);
  msg.handbrake = handbrake_pressed_;
  msg.locked = locked_;
  msg.multiplier = static_cast<float>(speed_);

  drive_info_pub_->publish(msg);
}

void TeleopDriveJoy::joint_states_callback(const sensor_msgs::msg::JointState::SharedPtr joint_state_msg)
{
  // get max effort of joints in rumble_joints
  double max_effort = 0;
  const std::vector<std::string>& names = joint_state_msg->name;
  for (std::string& joint_name : params_.rumble_joints)
  {
    auto iterator = std::find(names.begin(), names.end(), joint_name);
    if (iterator != names.end())
    {
      const double effort = std::abs(joint_state_msg->effort[std::distance(names.begin(), iterator)]);
      if (effort > max_effort)
      {
        max_effort = effort;
      }
    }
  }

  // map max_effort to values from 0 to 1, assuming its min = rumble_range[0] and max = rumble_range[1]
  double rumble_intensity = std::clamp((max_effort - params_.rumble_range[0])
    / (params_.rumble_range[1] - params_.rumble_range[0]), 0.0, 1.0);

  if (rumble_intensity > 0)
  {
    if (not start_rumble_)
    {
      start_rumble_ = this->now();
    }
  }
  else
  {
    start_rumble_.reset();
  }

  RCLCPP_DEBUG(this->get_logger(), "Game pad rumble calculations: rumble_intensity = %.2f, max_effort = %.2f", rumble_intensity, max_effort);

  // don't rumble if rumble_delay has not yet passed
  if (not start_rumble_ or start_rumble_.value()
    > this->now() - rclcpp::Duration(std::chrono::milliseconds(params_.rumble_delay)))
  {
    rumble_intensity = 0;
  }

  sensor_msgs::msg::JoyFeedback msg {};
  msg.type = sensor_msgs::msg::JoyFeedback::TYPE_RUMBLE;
  msg.id = 0;
  msg.intensity = static_cast<float>(rumble_intensity);
  joy_feedback_pub_->publish(msg);
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