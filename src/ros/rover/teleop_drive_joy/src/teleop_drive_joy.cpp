/**
 * @file teleop_drive_joy.cpp
 * @brief Teleop Drive Joy node to translate Joy messages from /joy to Drive commands
 * Edited by Kabi, Rohit, Victor
 */

#include "teleop_drive_joy/teleop_drive_joy.hpp"

using namespace std::chrono_literals;

namespace
{
  constexpr auto DEFAULT_INPUT_TOPIC = "/joy";
  constexpr auto DEFAULT_OUTPUT_TOPIC = "/drive_input";
  constexpr auto DEFAULT_OUTPUT_TOPIC_TWIST = "/cmd_vel";
  constexpr auto DEFAULT_OUTPUT_TOPIC_INFO = "/drive_info";
  constexpr auto BUTTON_DEBOUNCE_INTERVAL = std::chrono::milliseconds(200);
}

using std::placeholders::_1;

namespace teleop_drive_joy
{

  TeleopDriveJoy::TeleopDriveJoy(const rclcpp::NodeOptions &options)
      : Node("teleop_drive_joy_node", options)
  {
    drive_input_pub = this->create_publisher<nova_interfaces::msg::DriveInputStamped>(DEFAULT_OUTPUT_TOPIC, 50);
    cmd_vel_pub = this->create_publisher<geometry_msgs::msg::TwistStamped>(DEFAULT_OUTPUT_TOPIC_TWIST, 50);
    drive_info_pub = this->create_publisher<nova_interfaces::msg::DriveInfo>(DEFAULT_OUTPUT_TOPIC_INFO, 50);

    joy_sub = this->create_subscription<sensor_msgs::msg::Joy>(
        DEFAULT_INPUT_TOPIC, rclcpp::QoS(10), std::bind(&TeleopDriveJoy::joyCallback, this, _1));

    switch_controller_client = this->create_client<controller_manager_msgs::srv::SwitchController>("/controller_manager/switch_controller");
    pivot_drive_client = this->create_client<rcl_interfaces::srv::SetParameters>("/pivot_drive_controller/set_parameters");
    strafe_client = this->create_client<rcl_interfaces::srv::SetParameters>("/strafe_controller/set_parameters");
    nova_diff_drive_client = this->create_client<rcl_interfaces::srv::SetParameters>("/nova_diff_drive_controller/set_parameters");

    control_mode = ControlMode::PIVOT_DRIVE;
  }

  void TeleopDriveJoy::initializeParams()
  {
    param_listener_ = std::make_shared<ParamListener>(this->shared_from_this());
    params_ = param_listener_->get_params();

    if (param_listener_->is_old(params_))
    {
      params_ = param_listener_->get_params();
    }
    speed = params_.controllers_map.at(modeToController(control_mode)).scale_linear_x;
  }

  void TeleopDriveJoy::joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    handleButtonCallbacks(joy_msg);

    if (!current_state.locked)
    {
      sendDriveCommand(joy_msg);
    }
    else
    {
      sendHaltCommand();
    }
  }

  void TeleopDriveJoy::sendDriveCommand(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    auto controller_params = params_.controllers_map.at(modeToController(control_mode));

    double angular = joy_msg->axes[controller_params.axis_angular_z] * controller_params.scale_angular_z;
    double linear = joy_msg->axes[controller_params.axis_linear_x] * speed;

    if (current_state.autonomous_mode)
    {
      auto cmd_vel_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
      cmd_vel_msg->twist.angular.z = angular;
      cmd_vel_msg->twist.linear.x = linear;
      cmd_vel_msg->header.stamp = this->now();

      cmd_vel_pub->publish(std::move(cmd_vel_msg));
    }
    else
    {
      auto drive_input_msg = std::make_unique<nova_interfaces::msg::DriveInputStamped>();

      drive_input_msg->drive_input.radius = angular == 0 ? INFINITY : (1.0 / std::pow(std::abs(angular), 2)) - 1;
      drive_input_msg->drive_input.direction = angular > 0 ? -1 : angular < 0 ? 1
                                                                              : 0;
      drive_input_msg->drive_input.speed = linear;
      drive_input_msg->header.stamp = this->now();

      drive_input_pub->publish(std::move(drive_input_msg));
    }

    sent_lock_msg = false;
    previous_state = current_state;
    current_state.drive_mode = controlModeToDriveMode(control_mode);

    auto drive_info_msg = std::make_unique<nova_interfaces::msg::DriveInfo>();
    drive_info_msg->locked = current_state.locked;
    drive_info_msg->autonomous_mode = current_state.autonomous_mode;
    drive_info_msg->drive_mode = current_state.drive_mode;

    drive_info_pub->publish(std::move(drive_info_msg));
  }

  void TeleopDriveJoy::sendHaltCommand()
  {
    if (!current_state.autonomous_mode)
    {
      if (!sent_lock_msg)
      {
        auto drive_input_msg = std::make_unique<nova_interfaces::msg::DriveInputStamped>();
        drive_input_msg->drive_input.radius = INFINITY;
        drive_input_pub->publish(std::move(drive_input_msg));

        auto drive_info_msg = std::make_unique<nova_interfaces::msg::DriveInfo>();
        drive_info_msg->locked = current_state.locked;
        drive_info_msg->autonomous_mode = current_state.autonomous_mode;
        drive_info_msg->drive_mode = current_state.drive_mode;

        drive_info_pub->publish(std::move(drive_info_msg));
        sent_lock_msg = true;
      }
    }
    else
    {
      if (!sent_lock_msg)
      {
      auto cmd_vel_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
      cmd_vel_pub->publish(std::move(cmd_vel_msg));

      auto drive_info_msg = std::make_unique<nova_interfaces::msg::DriveInfo>();
      drive_info_msg->locked = current_state.locked;
      drive_info_msg->autonomous_mode = current_state.autonomous_mode;
      drive_info_msg->drive_mode = current_state.drive_mode;

      drive_info_pub->publish(std::move(drive_info_msg));
      sent_lock_msg = true;
      }
    }
  }

  void TeleopDriveJoy::handleButtonCallbacks(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
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
    if (isDebounced(joy_msg->buttons[params_.button_unlock], params_.button_unlock) && current_state.locked)
    {
      current_state.locked = false;
      RCLCPP_INFO(this->get_logger(), "BUTTON: unlock");
    }
    else if (isDebounced(joy_msg->buttons[params_.button_lock], params_.button_lock) && !current_state.locked)
    {
      current_state.locked = true;
      RCLCPP_INFO(this->get_logger(), "BUTTON: lock");
    }

    // Autonomous and Manual
    if (isDebounced(joy_msg->buttons[params_.button_autonomous_control], params_.button_autonomous_control) && !current_state.autonomous_mode)
    {
      current_state.autonomous_mode = true;
      setEnableTwistCmdForController(pivot_drive_client, true);
      setEnableTwistCmdForController(strafe_client, true);
	    setEnableTwistCmdForController(nova_diff_drive_client, true);
      RCLCPP_INFO(this->get_logger(), "BUTTON: autonomous_control");
    }
    else if (isDebounced(joy_msg->buttons[params_.button_manual_control], params_.button_manual_control) && current_state.autonomous_mode)
    {
      current_state.autonomous_mode = false;
      setEnableTwistCmdForController(pivot_drive_client, false);
      setEnableTwistCmdForController(strafe_client, false);
      setEnableTwistCmdForController(nova_diff_drive_client, false);
      RCLCPP_INFO(this->get_logger(), "BUTTON: manual_control");
    }

    // Controller Switches
    if (isDebounced(joy_msg->buttons[params_.button_pivot_drive_controller], params_.button_pivot_drive_controller) && control_mode != ControlMode::PIVOT_DRIVE)
    {
      RCLCPP_INFO(this->get_logger(), "BUTTON: pivot_drive_mode");
      switchController(ControlMode::PIVOT_DRIVE);
    }
    else if (isDebounced(joy_msg->buttons[params_.button_strafe_controller], params_.button_strafe_controller) && control_mode != ControlMode::STRAFE_DRIVE)
    {
      RCLCPP_INFO(this->get_logger(), "BUTTON: strafe_mode");
      switchController(ControlMode::STRAFE_DRIVE);
    }
    else if (isDebounced(joy_msg->buttons[params_.button_nova_diff_drive_controller], params_.button_nova_diff_drive_controller) && control_mode != ControlMode::DIFF_DRIVE)
    {
      RCLCPP_INFO(this->get_logger(), "BUTTON: diff_drive_mode");
      switchController(ControlMode::DIFF_DRIVE);
    }

    if (current_state.locked)
      return;

    // Need for Speed (only when controller is unlock'd)
    if (isDebounced(joy_msg->axes[params_.axis_speed_change_fine], params_.axis_speed_change_fine + 30) != 0) // 30 is magic number added here to avoid conflicts with other buttons on the Debouncer
    {
      speed = std::clamp(speed + joy_msg->axes[params_.axis_speed_change_fine] * params_.speed_change_fine_val, params_.speed_limit_min, params_.speed_limit_max);
      RCLCPP_INFO(this->get_logger(), "Speed: %f", speed);
    }
    else if (isDebounced(joy_msg->axes[params_.axis_speed_change_coarse], params_.axis_speed_change_coarse + 30) != 0)
    {
      speed = std::clamp(speed + joy_msg->axes[params_.axis_speed_change_coarse] * params_.speed_change_coarse_val, params_.speed_limit_min, params_.speed_limit_max);
      RCLCPP_INFO(this->get_logger(), "Speed: %f", speed);
    }
  }

  void TeleopDriveJoy::switchController(const ControlMode requested_control_mode)
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

  void TeleopDriveJoy::setEnableTwistCmdForController(const std::shared_ptr<rclcpp::Client<rcl_interfaces::srv::SetParameters>> &client, bool enable)
  {
    if (!client->service_is_ready())
    {
      RCLCPP_ERROR(this->get_logger(), "Service is not ready for client.");
      return;
    }

    // Create the parameter request
    auto request = std::make_shared<rcl_interfaces::srv::SetParameters::Request>();

    // Construct the parameter manually
    rcl_interfaces::msg::Parameter param;
    param.name = "enable_twist_cmd";
    param.value.type = rcl_interfaces::msg::ParameterType::PARAMETER_BOOL;
    param.value.bool_value = enable; // Use rclcpp::ParameterValue to set the bool value
    // Add the parameter to the request
    request->parameters.push_back(param);

    // TODO: Add confirmation of parameter change
    auto future = client->async_send_request(request);
  }

}

int main(int argc, char *argv[])
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<teleop_drive_joy::TeleopDriveJoy>();
  node->initializeParams();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}