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
using drive_interfaces::msg::DriveInfo;

namespace teleop_drive_joy
{
  ButtonHandler::ButtonHandler(const int button_index, const std::chrono::milliseconds debounce_timeout,
    const std::function<void(const sensor_msgs::msg::Joy::SharedPtr)> on_down,
    const std::function<void(const sensor_msgs::msg::Joy::SharedPtr)> on_up)
      : debounce_timeout_(debounce_timeout)
      , on_down_(on_down)
      , is_pressed_([button_index](const sensor_msgs::msg::Joy::SharedPtr joy_msg) -> bool
        { return static_cast<bool>(joy_msg->buttons[button_index]); })
      , on_up_(on_up)
  {
  }

  ButtonHandler::ButtonHandler(const std::function<bool(const sensor_msgs::msg::Joy::SharedPtr)> is_pressed,
    const std::chrono::milliseconds debounce_timeout,
    const std::function<void(const sensor_msgs::msg::Joy::SharedPtr)> on_down,
    const std::function<void(const sensor_msgs::msg::Joy::SharedPtr)> on_up)
      : debounce_timeout_(debounce_timeout)
      , is_pressed_(is_pressed)
      , on_down_(on_down)
      , on_up_(on_up)
  {
  }

  bool ButtonHandler::is_timed_out(const rclcpp::Time& now)
  {
    if (last_pressed_.has_value())
    {
      return (now - last_pressed_.value()) <= debounce_timeout_;
    }
    else
    {
      last_pressed_ = now;
      return false;
    }
  }

  void ButtonHandler::update(const sensor_msgs::msg::Joy::SharedPtr joy_msg, const rclcpp::Time now)
  {
    if (is_timed_out(now))
    {
      return;
    }

    if (is_pressed_(joy_msg) and not pressed_)
    {
      pressed_ = true;
      last_pressed_ = now;
      if (on_down_) { on_down_(joy_msg); }
    }
    else if (not is_pressed_(joy_msg) and pressed_)
    {
      pressed_ = false;
      if (on_up_) { on_up_(joy_msg); }
    }
  }

  TeleopDriveJoy::TeleopDriveJoy(const rclcpp::NodeOptions& options)
  : Node("teleop_drive_joy_node", options)
  , sent_lock_msg_(false)
  , locked_(true)
  , drive_mode_(DriveMode::PIVOT)
  , handbrake_(false)
  , hold_position_(false)
  , connected_(false)
  , autonomous_mode_(false)
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

  for (const auto& client : {pivot_drive_client_, holonomic_drive_client_, strafe_client_, diff_drive_client_})
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
  drive_info_pub_ = this->create_publisher<DriveInfo>(params_.drive_info_topic, 1);

  switch_controller_client_ = this->create_client<controller_manager_msgs::srv::SwitchController>(
    "/controller_manager/switch_controller");
  pivot_drive_client_ = this->create_client<rcl_interfaces::srv::SetParameters>(
    "/pivot_drive_controller/set_parameters");
  holonomic_drive_client_ = this->create_client<rcl_interfaces::srv::SetParameters>(
  "/holonomic_drive_controller/set_parameters");
  strafe_client_ = this->create_client<rcl_interfaces::srv::SetParameters>(
    "/strafe_drive_controller/set_parameters");
  diff_drive_client_ = this->create_client<rcl_interfaces::srv::SetParameters>(
    "/diff_drive_controller/set_parameters");

  set_drive_status_client_ = this->create_client<drive_interfaces::srv::DriveStatus>(
    "/drive_controller/set_drive_status");
}

void TeleopDriveJoy::map_button_callbacks()
{
  const std::chrono::milliseconds debounce_duration { params_.button_debounce_time };

  button_handlers_.emplace_back(params_.button_unlock, debounce_duration,
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (locked_)
    {
      locked_ = false;
      send_drive_info();
      RCLCPP_INFO_STREAM(this->get_logger(), C_SUCCESS << "Gamepad unlocked" << C_END);
    }
  });
  button_handlers_.emplace_back(params_.button_lock, debounce_duration,
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (!locked_)
    {
      locked_ = true;
      send_drive_info();
      RCLCPP_INFO_STREAM(this->get_logger(), C_FAIL << "Gamepad locked" << C_END);
    }
  });

  button_handlers_.emplace_back(params_.button_autonomous_mode, debounce_duration,
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    set_autonomous_mode_for_controllers(true);
    RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << "Autonomous mode" << C_END);
  });
  button_handlers_.emplace_back(params_.button_manual_mode, debounce_duration,
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    set_autonomous_mode_for_controllers(false);
    RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << "Manual mode" << C_END);
  });

  button_handlers_.emplace_back(params_.button_pivot_drive_controller, debounce_duration,
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    switch_controller(DriveMode::PIVOT);
  });
  button_handlers_.emplace_back(params_.button_holonomic_drive_controller, debounce_duration,
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    switch_controller(DriveMode::HOLONOMIC);
  });
  button_handlers_.emplace_back(params_.button_strafe_drive_controller, debounce_duration,
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    switch_controller(DriveMode::STRAFE);
  });
  button_handlers_.emplace_back(params_.button_diff_drive_controller, debounce_duration,
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    switch_controller(DriveMode::DIFF);
  });

  button_handlers_.emplace_back(
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    return std::abs(joy_msg->axes[params_.axis_hold_position]) > params_.trigger_pressed_threshold;
  },
    debounce_duration,
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    set_hold_position(true);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "Hold position " << C_SUCCESS << "enabled"<< C_END);
  },
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    set_hold_position(false);
    RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "Hold position " << C_FAIL << "disabled"<< C_END);
  });

  button_handlers_.emplace_back(
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    return std::abs(joy_msg->axes[params_.axis_handbrake]) > params_.trigger_pressed_threshold;
  },
    debounce_duration,
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    handbrake_ = true;
    send_drive_info();
    RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "Handbrake " C_SUCCESS << "activated" << C_END);
  },
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    handbrake_ = false;
    send_drive_info();
    RCLCPP_INFO_STREAM(this->get_logger(), C_INFO << "Handbrake " C_FAIL << "deactivated" << C_END);
  });

  auto change_speed = [this](double speed_change)
  {
    speed_ = std::clamp(
      speed_ + speed_change,
      params_.speed_limit_min, params_.speed_limit_max);
    send_drive_info();
    RCLCPP_INFO_STREAM(this->get_logger(), C_SPEED << "Speed: " << speed_ << C_END);
  };
  button_handlers_.emplace_back(params_.button_speed_decrease_fine, debounce_duration,
    [this, change_speed](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    change_speed(-params_.speed_change_fine_val);
  });
  button_handlers_.emplace_back(params_.button_speed_increase_fine, debounce_duration,
    [this, change_speed](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    change_speed(params_.speed_change_fine_val);
  });
  button_handlers_.emplace_back(params_.button_speed_decrease_coarse, debounce_duration,
    [this, change_speed](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    change_speed(-params_.speed_change_coarse_val);
  });
  button_handlers_.emplace_back(params_.button_speed_increase_coarse, debounce_duration,
    [this, change_speed](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    change_speed(params_.speed_change_coarse_val);
  });
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
  set_connected(true);
  connection_timer_->reset();
}

void TeleopDriveJoy::handle_button_callbacks(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
{
  auto now = this->now();

  for (auto& handler : button_handlers_)
  {
    handler.update(joy_msg, now);
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
  if (drive_mode_ == DriveMode::HOLONOMIC)
  {
    angular *= speed_;
  }
  angular = std::clamp(angular, -controller_params.limit_angular, controller_params.limit_angular);

	if (handbrake_)
	{
    linear.first *= params_.handbrake_speed_multiplier;
    linear.second *= params_.handbrake_speed_multiplier;
    if (drive_mode_ == DriveMode::HOLONOMIC)
    {
      angular *= params_.handbrake_speed_multiplier;
    }
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

  switch_controller_client_->prune_pending_requests();

  auto request = std::make_shared<controller_manager_msgs::srv::SwitchController::Request>();
  request->activate_controllers.emplace_back(activate_controller);
  request->deactivate_controllers.emplace_back(deactivate_controller);
  request->strictness = 2;
  request->activate_asap = true;

  auto future = switch_controller_client_->async_send_request(request,
    [this](rclcpp::Client<controller_manager_msgs::srv::SwitchController>::SharedFuture future)
  {
      auto response = future.get();

      if (response->ok)
      {
        set_hold_position(hold_position_);
      }
      else
      {
        RCLCPP_ERROR_STREAM(this->get_logger(), C_FAIL << "Failed to switch controller with message: " << response->message << C_END);
      }
  });

  drive_mode_ = requested_control_mode;
  send_drive_info();
  RCLCPP_INFO_STREAM(this->get_logger(), C_MODE << pretty_print_mode(drive_mode_) << C_END);
}

void TeleopDriveJoy::set_hold_position(bool enable)
{
  if (!set_drive_status_client_->service_is_ready())
  {
    RCLCPP_ERROR_STREAM(this->get_logger(), C_FAIL << "set_drive_status server not available. Not updating drive status" << C_END);
    return;
  }

  auto request = std::make_shared<drive_interfaces::srv::DriveStatus::Request>();
  request->hold_position = enable;

  auto result = set_drive_status_client_->async_send_request(request);

  hold_position_ = enable;
  send_drive_info();
}

void TeleopDriveJoy::set_connected(bool connected)
{
  if (connected != connected_)
  {
    connected_ = connected;
    send_drive_info();

    if (connected_) { RCLCPP_ERROR_STREAM(this->get_logger(), C_INFO << "Gamepad " << C_SUCCESS << "connected." << C_END); }
    else { RCLCPP_ERROR_STREAM(this->get_logger(), C_INFO << "Gamepad " << C_FAIL << "disconnected." << C_END); }
  }
}

void TeleopDriveJoy::send_drive_info()
{
  auto drive_info_msg = std::make_unique<DriveInfo>();
  drive_info_msg->multiplier = static_cast<float>(speed_);
  drive_info_msg->locked = locked_;
  drive_info_msg->autonomous_mode = autonomous_mode_;
  drive_info_msg->connected = connected_;
  drive_info_msg->drive_mode = static_cast<uint8_t>(drive_mode_);
  drive_info_msg->handbrake = handbrake_;
  drive_info_msg->hold_position = hold_position_;

  drive_info_pub_->publish(std::move(drive_info_msg));
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