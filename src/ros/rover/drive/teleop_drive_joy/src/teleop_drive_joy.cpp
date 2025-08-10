/**
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * DESCRIPTION: Convert joystick input into drive or twist messages
 * to be received by controllers (pivot, strafe, etc).
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * NODE: teleop_drive_joy
 * TOPICS:
 *  - subscriber: /joy               [sensor_msgs/msg/Joy]
 *  - publisher:  /cmd_vel           [geometry_msgs/msg/TwistStamped]
 * SERVICES:
 *  - client:     /controller_manager/switch_controller
 * [controller_manager_msgs/srv/SwitchController]
 *  - client:     /pivot_drive_controller/set_parameters       [rcl_interfaces/srv/SetParameters]
 *  - client:     /strafe_drive_controller/set_parameters            [rcl_interfaces/srv/SetParameters]
 *  - client:     /diff_drive_controller/set_parameters   [rcl_interfaces/srv/SetParameters]
 * ACTIONS: None
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * PACKAGE:    teleop_drive_joy
 * AUTHORS:	   Kabi, Terry Tian
 * CREATION:	 ?
 * EDITED:		 15/06/2025
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <utility>
#include <tuple>

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
      RCLCPP_ERROR(this->get_logger(), "Service is not ready for client.");
      continue;
    }

    static_cast<void>(client->async_send_request(request));
  }

}  // namespace teleop_drive_joy

void TeleopDriveJoy::initialize()
{
  initializeParams();
  initializeInterfaces();
  mapButtonCallbacks();
  RCLCPP_INFO(
    this->get_logger(), "Initialized with control mode: %s, speed: %s",
    prettyPrintMode(drive_mode_).c_str(), std::to_string(speed_).c_str());
}

void TeleopDriveJoy::initializeParams()
{
  param_listener_ = std::make_shared<ParamListener>(this->shared_from_this());
  params_ = param_listener_->get_params();

  if (param_listener_->is_old(params_))
  {
    params_ = param_listener_->get_params();
  }
  speed_ = params_.initial_speed;
}

void TeleopDriveJoy::initializeInterfaces()
{
  joy_sub_ = this->create_subscription<sensor_msgs::msg::Joy>(
    params_.input_topic, rclcpp::QoS(10), std::bind(&TeleopDriveJoy::joyCallback, this, _1));
  cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::TwistStamped>(params_.output_topic, 50);

  switch_controller_client_ = this->create_client<controller_manager_msgs::srv::SwitchController>(
    "/controller_manager/switch_controller");
  pivot_drive_client_ = this->create_client<rcl_interfaces::srv::SetParameters>(
    "/pivot_drive_controller/set_parameters");
  strafe_client_ =
    this->create_client<rcl_interfaces::srv::SetParameters>("/strafe_drive_controller/set_parameters");
  diff_drive_client_ = this->create_client<rcl_interfaces::srv::SetParameters>(
    "/diff_drive_controller/set_parameters");
}

void TeleopDriveJoy::mapButtonCallbacks()
{
  button_callbacks_[params_.button_unlock] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (locked_)
    {
      locked_ = false;
      RCLCPP_INFO(this->get_logger(), "BUTTON: unlock");
    }
  };
  button_callbacks_[params_.button_lock] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (!locked_)
    {
      locked_ = true;
      RCLCPP_INFO(this->get_logger(), "BUTTON: lock");
    }
  };
  button_callbacks_[params_.button_autonomous_mode] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    set_autonomous_mode_for_controllers(true);
    RCLCPP_INFO(this->get_logger(), "BUTTON: autonomous_mode");
  };
  button_callbacks_[params_.button_manual_mode] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    set_autonomous_mode_for_controllers(false);
    RCLCPP_INFO(this->get_logger(), "BUTTON: manual_mode");
  };
  button_callbacks_[params_.button_pivot_drive_controller] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (drive_mode_ != DriveMode::PIVOT)
    {
      switchController(DriveMode::PIVOT);
      RCLCPP_INFO(this->get_logger(), "BUTTON: pivot_drive");
    }
  };
  button_callbacks_[params_.button_holonomic_drive_controller] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (drive_mode_ != DriveMode::HOLONOMIC)
    {
      switchController(DriveMode::HOLONOMIC);
      RCLCPP_INFO(this->get_logger(), "BUTTON: holonomic_drive");
    }
  };
  button_callbacks_[params_.button_strafe_drive_controller] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (drive_mode_ != DriveMode::STRAFE)
    {
      switchController(DriveMode::STRAFE);
      RCLCPP_INFO(this->get_logger(), "BUTTON: strafe_drive");
    }
  };
  button_callbacks_[params_.button_diff_drive_controller] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (drive_mode_ != DriveMode::DIFF)
    {
      switchController(DriveMode::DIFF);
      RCLCPP_INFO(this->get_logger(), "BUTTON: diff_drive");
    }
  };
  // mark axis buttons as negative
  button_callbacks_[-(params_.axis_speed_change_fine)] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (!locked_)
    {
      speed_ = std::clamp(
        speed_ + joy_msg->axes[params_.axis_speed_change_fine] * params_.speed_change_fine_val,
        params_.speed_limit_min, params_.speed_limit_max);
      RCLCPP_INFO(this->get_logger(), "Speed: %f", speed_);
    }
  };
  button_callbacks_[-(params_.axis_speed_change_coarse)] =
    [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    if (!locked_)
    {
      speed_ = std::clamp(
        speed_ + joy_msg->axes[params_.axis_speed_change_coarse] * params_.speed_change_coarse_val,
        params_.speed_limit_min, params_.speed_limit_max);
      RCLCPP_INFO(this->get_logger(), "Speed: %f", speed_);
    }
  };
}

void TeleopDriveJoy::joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
{
  handleButtonCallbacks(joy_msg);

  if (!locked_)
  {
    sendDriveCommand(joy_msg);
  }
  else
  {
    sendHaltCommand();
  }
}

void TeleopDriveJoy::handleButtonCallbacks(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
{
  auto now = this->now();

  auto isPressedAndDebounced = [this, &now, joy_msg](int button_index)
  {
    bool pressed = (button_index >= 0 && joy_msg->buttons[button_index]) ||
                   (button_index < 0 && joy_msg->axes[std::abs(button_index)]);
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

void TeleopDriveJoy::sendDriveCommand(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
{
  const auto& controller_params = params_.controllers_map.at(modeToController(drive_mode_));

  auto [linear, angular] = snapped_joy_axes(joy_msg);
  linear.first *= controller_params.scale_linear * speed_;
  linear.second *= controller_params.scale_linear * speed_;
  angular *= controller_params.scale_angular;

  auto cmd_vel_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
  cmd_vel_msg->twist.linear.x = linear.first;
  cmd_vel_msg->twist.linear.y = linear.second;
  cmd_vel_msg->twist.angular.z = angular;
  cmd_vel_msg->header.stamp = this->now();

  cmd_vel_pub_->publish(std::move(cmd_vel_msg));

  sent_lock_msg_ = false;
}

void TeleopDriveJoy::sendHaltCommand()
{
  if (sent_lock_msg_) return;

  auto cmd_vel_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
  cmd_vel_pub_->publish(std::move(cmd_vel_msg));

  sent_lock_msg_ = true;
}

void TeleopDriveJoy::switchController(const DriveMode requested_control_mode)
{
  RCLCPP_INFO(
    this->get_logger(), "Changing from %s to %s", prettyPrintMode(drive_mode_).c_str(),
    prettyPrintMode(requested_control_mode).c_str());

  std::string deactivate_controller = modeToController(drive_mode_);
  std::string activate_controller = modeToController(requested_control_mode);

  if (!switch_controller_client_->service_is_ready())
  {
    RCLCPP_ERROR(this->get_logger(), "Controller manager service not available.");
    return;
  }

  auto request = std::make_shared<controller_manager_msgs::srv::SwitchController::Request>();
  request->activate_controllers.emplace_back(activate_controller);
  request->deactivate_controllers.emplace_back(deactivate_controller);
  request->strictness = 2;
  request->activate_asap = true;

  auto future = switch_controller_client_->async_send_request(request);

  drive_mode_ = requested_control_mode;
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