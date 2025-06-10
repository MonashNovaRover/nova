/**
 * @file teleop_drive_joy.cpp
 * @brief Teleop Drive Joy node to translate Joy messages from /joy to Drive commands
 * Edited by Kabi, Rohit, Victor
 */

#include "teleop_drive_joy/teleop_drive_joy.hpp"

using namespace std::chrono_literals;

using std::placeholders::_1;

namespace teleop_drive_joy
{

  TeleopDriveJoy::TeleopDriveJoy(const rclcpp::NodeOptions &options)
  : Node("teleop_drive_joy_node", options)
  {
  }
  
  void TeleopDriveJoy::initialize()
  {
    initializeParams();
    initializeInterfaces();
    mapButtonCallbacks();
    RCLCPP_INFO(
      this->get_logger(), "Initialized with control mode: %s, speed: %s",
      prettyPrintMode(control_mode_).c_str(), std::to_string(speed_).c_str()
    );
  }
  
  void TeleopDriveJoy::initializeParams()
  {
    param_listener_ = std::make_shared<ParamListener>(this->shared_from_this());
    params_ = param_listener_->get_params();

    if (param_listener_->is_old(params_))
    {
      params_ = param_listener_->get_params();
    }
    speed_ = params_.controllers_map.at(modeToController(control_mode_)).scale_linear_x;
  }

  void TeleopDriveJoy::initializeInterfaces()
  {
    drive_input_pub_ = this->create_publisher<nova_interfaces::msg::DriveInputStamped>(params_.output_topic, 50);
    cmd_vel_pub_ = this->create_publisher<geometry_msgs::msg::TwistStamped>(params_.output_topic_twist, 50);
    drive_info_pub_ = this->create_publisher<nova_interfaces::msg::DriveInfo>(params_.output_topic_info, 50);

    joy_sub_ = this->create_subscription<sensor_msgs::msg::Joy>(
        params_.input_topic, rclcpp::QoS(10), std::bind(&TeleopDriveJoy::joyCallback, this, _1));

    switch_controller_client_ = this->create_client<controller_manager_msgs::srv::SwitchController>("/controller_manager/switch_controller");
    pivot_drive_client_ = this->create_client<rcl_interfaces::srv::SetParameters>("/pivot_drive_controller/set_parameters");
    strafe_client_ = this->create_client<rcl_interfaces::srv::SetParameters>("/strafe_controller/set_parameters");
    nova_diff_drive_client_ = this->create_client<rcl_interfaces::srv::SetParameters>("/nova_diff_drive_controller/set_parameters");
  }

  void TeleopDriveJoy::mapButtonCallbacks()
  {
    button_callbacks_[params_.button_unlock] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
      if (current_state_.locked)
      {
        current_state_.locked = false;
        RCLCPP_INFO(this->get_logger(), "BUTTON: unlock");
      }
    };
    button_callbacks_[params_.button_lock] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
      if (!current_state_.locked)
      {
        current_state_.locked = true;
        RCLCPP_INFO(this->get_logger(), "BUTTON: lock");
      }
    };
    button_callbacks_[params_.button_autonomous_control] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
      if (!current_state_.autonomous_mode)
      {
        current_state_.autonomous_mode = true;
        setEnableTwistCmdForController(pivot_drive_client_, true);
        setEnableTwistCmdForController(strafe_client_, true);
        setEnableTwistCmdForController(nova_diff_drive_client_, true);
        RCLCPP_INFO(this->get_logger(), "BUTTON: autonomous_control");
      }
    };
    button_callbacks_[params_.button_manual_control] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
      if (current_state_.autonomous_mode)
      {
        current_state_.autonomous_mode = false;
        setEnableTwistCmdForController(pivot_drive_client_, false);
        setEnableTwistCmdForController(strafe_client_, false);
        setEnableTwistCmdForController(nova_diff_drive_client_, false);
        RCLCPP_INFO(this->get_logger(), "BUTTON: manual_control");
      }
    };
    button_callbacks_[params_.button_pivot_drive_controller] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
      if (control_mode_ != nova_interfaces::msg::DriveInfo::PIVOT)
      {
        switchController(nova_interfaces::msg::DriveInfo::PIVOT);
        RCLCPP_INFO(this->get_logger(), "BUTTON: pivot_drive");
      }
    };
    button_callbacks_[params_.button_strafe_controller] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
      if (control_mode_ != nova_interfaces::msg::DriveInfo::STRAFE)
      {
        switchController(nova_interfaces::msg::DriveInfo::STRAFE);
        RCLCPP_INFO(this->get_logger(), "BUTTON: strafe_drive");
      }
    };
    button_callbacks_[params_.button_nova_diff_drive_controller] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
      if (control_mode_ != nova_interfaces::msg::DriveInfo::DIFF)
      {
        switchController(nova_interfaces::msg::DriveInfo::DIFF);
        RCLCPP_INFO(this->get_logger(), "BUTTON: nova_diff_drive");
      }
    };
    // mark axis buttons as negative
    button_callbacks_[-(params_.axis_speed_change_fine)] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
      if (!current_state_.locked)
      {
        speed_ = std::clamp(
          speed_ + joy_msg->axes[params_.axis_speed_change_fine] * params_.speed_change_fine_val,
          params_.speed_limit_min, params_.speed_limit_max
        );
        RCLCPP_INFO(this->get_logger(), "Speed: %f", speed_);
      }
    };
    button_callbacks_[-(params_.axis_speed_change_coarse)] = [this](const sensor_msgs::msg::Joy::SharedPtr joy_msg) {
      if (!current_state_.locked)
      {
        speed_ = std::clamp(
          speed_ + joy_msg->axes[params_.axis_speed_change_coarse] * params_.speed_change_coarse_val,
          params_.speed_limit_min, params_.speed_limit_max
        );
        RCLCPP_INFO(this->get_logger(), "Speed: %f", speed_);
      }
    };
  }

  void TeleopDriveJoy::joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    handleButtonCallbacks(joy_msg);

    if (!current_state_.locked)
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

    auto isPressedAndDebounced = [this, &now, joy_msg](int button_index) {
      bool pressed = (button_index >= 0 && joy_msg->buttons[button_index]) || (button_index < 0 && joy_msg->axes[std::abs(button_index)]);
      bool debounced = last_button_press_time_.find(button_index) == last_button_press_time_.end() ||
                      now - last_button_press_time_[button_index] > rclcpp::Duration(std::chrono::milliseconds(params_.button_debounce_time));
      return pressed && debounced;
    };

    for (const auto &[button_index, button_callback] : button_callbacks_)
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
    auto controller_params = params_.controllers_map.at(modeToController(control_mode_));

    double angular = joy_msg->axes[controller_params.axis_angular_y] * controller_params.scale_angular_y;
    double linear = joy_msg->axes[controller_params.axis_linear_x] * speed_;

    if (current_state_.autonomous_mode)
    {
      auto cmd_vel_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
      cmd_vel_msg->twist.angular.z = angular;
      cmd_vel_msg->twist.linear.x = linear;
      cmd_vel_msg->header.stamp = this->now();

      cmd_vel_pub_->publish(std::move(cmd_vel_msg));
    }
    else
    {
      auto drive_input_msg = std::make_unique<nova_interfaces::msg::DriveInputStamped>();

      drive_input_msg->drive_input.radius = angular == 0 ? INFINITY : (1.0 / std::pow(std::abs(angular), 2)) - 1;
      drive_input_msg->drive_input.direction = angular > 0 ? -1 : angular < 0 ? 1
                                                                              : 0;
      drive_input_msg->drive_input.speed = linear;
      drive_input_msg->header.stamp = this->now();

      drive_input_pub_->publish(std::move(drive_input_msg));
    }

    sent_lock_msg_ = false;
    previous_state_ = current_state_;
    current_state_.drive_mode = control_mode_;

    auto drive_info_msg = std::make_unique<nova_interfaces::msg::DriveInfo>();
    drive_info_msg->locked = current_state_.locked;
    drive_info_msg->autonomous_mode = current_state_.autonomous_mode;
    drive_info_msg->drive_mode = current_state_.drive_mode;

    drive_info_pub_->publish(std::move(drive_info_msg));
  }

  void TeleopDriveJoy::sendHaltCommand()
  {
    if (!current_state_.autonomous_mode)
    {
      if (!sent_lock_msg_)
      {
        auto drive_input_msg = std::make_unique<nova_interfaces::msg::DriveInputStamped>();
        drive_input_msg->drive_input.radius = INFINITY;
        drive_input_pub_->publish(std::move(drive_input_msg));

        auto drive_info_msg = std::make_unique<nova_interfaces::msg::DriveInfo>();
        drive_info_msg->locked = current_state_.locked;
        drive_info_msg->autonomous_mode = current_state_.autonomous_mode;
        drive_info_msg->drive_mode = current_state_.drive_mode;

        drive_info_pub_->publish(std::move(drive_info_msg));
        sent_lock_msg_ = true;
      }
    }
    else
    {
      if (!sent_lock_msg_)
      {
      auto cmd_vel_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
      cmd_vel_pub_->publish(std::move(cmd_vel_msg));

      auto drive_info_msg = std::make_unique<nova_interfaces::msg::DriveInfo>();
      drive_info_msg->locked = current_state_.locked;
      drive_info_msg->autonomous_mode = current_state_.autonomous_mode;
      drive_info_msg->drive_mode = current_state_.drive_mode;

      drive_info_pub_->publish(std::move(drive_info_msg));
      sent_lock_msg_ = true;
      }
    }
  }

  void TeleopDriveJoy::switchController(const uint requested_control_mode)
  {
    if (requested_control_mode == control_mode_)
      return;

    RCLCPP_INFO(this->get_logger(), "Changing from %s to %s",
                prettyPrintMode(control_mode_).c_str(),
                prettyPrintMode(requested_control_mode).c_str());

    std::string deactivate_controller = modeToController(control_mode_);
    std::string activate_controller = modeToController(requested_control_mode);

    if (!switch_controller_client_->service_is_ready()) {
        RCLCPP_ERROR(this->get_logger(), "Controller manager service not available.");
        return;
    }

    auto request = std::make_shared<controller_manager_msgs::srv::SwitchController::Request>();
    request->activate_controllers.emplace_back(activate_controller);
    request->deactivate_controllers.emplace_back(deactivate_controller);
    request->strictness = 2;
    request->activate_asap = true;

    auto future = switch_controller_client_->async_send_request(request);

    control_mode_ = requested_control_mode;
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
  node->initialize();
  rclcpp::spin(node);
  rclcpp::shutdown();
  return 0;
}