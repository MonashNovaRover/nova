/**
 * @file teleop_drive_joy.cpp
 * @brief Teleop Drive Joy node to Translate Joy Messages from /joy to Drive Commands
 * @author Dylan Gonzalez
 * Last Edited by Kabi
 */
#include <cinttypes>
#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>

#include <rclcpp/rclcpp.hpp>
#include <rcutils/logging_macros.h>
#include <builtin_interfaces/msg/duration.hpp>

#include "teleop_drive_joy/teleop_drive_joy.hpp"

#define ROS_INFO_NAMED RCUTILS_LOG_INFO_NAMED
#define ROS_INFO_COND_NAMED RCUTILS_LOG_INFO_EXPRESSION_NAMED

using namespace std::chrono_literals;

namespace
{
  constexpr auto DEFAULT_INPUT_TOPIC = "/joy";
  constexpr auto DEFAULT_OUTPUT_TOPIC = "/drive_input";
  constexpr auto DEFAULT_OUTPUT_TOPIC_TWIST = "/cmd_vel";
  constexpr auto DEFAULT_OUTPUT_TOPIC_INFO = "/drive_info";
}

using std::placeholders::_1;
namespace teleop_drive_joy
{

  rclcpp::node_interfaces::NodeBaseInterface::SharedPtr TeleopDriveJoy::get_node_base_interface()
  {
    return node_->get_node_base_interface();
  }

  TeleopDriveJoy::TeleopDriveJoy(const rclcpp::NodeOptions &options) : node_{std::make_shared<rclcpp::Node>("teleop_drive_joy_node_", options)}
  {
    drive_input_pub = node_->create_publisher<nova_interfaces::msg::DriveInputStamped>(DEFAULT_OUTPUT_TOPIC, 50);
    cmd_vel_pub = node_->create_publisher<geometry_msgs::msg::TwistStamped>(DEFAULT_OUTPUT_TOPIC_TWIST, 50);
    drive_info_pub = node_->create_publisher<nova_interfaces::msg::DriveInfo>(DEFAULT_OUTPUT_TOPIC_INFO, 50);

    joy_sub = node_->create_subscription<sensor_msgs::msg::Joy>(DEFAULT_INPUT_TOPIC, rclcpp::QoS(10), std::bind(&TeleopDriveJoy::joyCallback, this, _1));

    switch_controller_client = node_->create_client<controller_manager_msgs::srv::SwitchController>("/controller_manager/switch_controller");

    param_listener_ = std::make_shared<ParamListener>(node_);
    params_ = param_listener_->get_params();

    if (param_listener_->is_old(params_))
    {
      params_ = param_listener_->get_params();
    }
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
      // When lock button is pressed, immediately send a single halt command to Stop the Rover
      sendHaltCommand();
    }
  }

  void TeleopDriveJoy::sendDriveCommand(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {

    auto controller_params = params_.controllers_map.at(modeToController(control_mode));

    double angular = joy_msg->axes[controller_params.axis_angular_z] * controller_params.scale_angular_z;
    double linear = joy_msg->axes[controller_params.axis_linear_x] * controller_params.scale_linear_x;

    if (current_state.autonomous_mode)
    {
      auto cmd_vel_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
      cmd_vel_msg->twist.angular.z = angular;
      cmd_vel_msg->twist.linear.x = linear;
      cmd_vel_msg->header.stamp = node_->now();

      cmd_vel_pub->publish(std::move(cmd_vel_msg));
    }
    else
    {
      auto drive_input_msg = std::make_unique<nova_interfaces::msg::DriveInputStamped>();

      drive_input_msg->drive_input.radius = angular == 0 ? INFINITY : (1.0 / pow(abs(angular), 2)) - 1;
      drive_input_msg->drive_input.direction = angular > 0 ? -1 : angular < 0 ? 1
                                                                              : 0;
      drive_input_msg->drive_input.speed = linear;
      drive_input_msg->header.stamp = node_->now();

      drive_input_pub->publish(std::move(drive_input_msg));
    }

    sent_lock_msg = false;
    previous_state = current_state;
    current_state.drive_mode = controlModeToDriveMode(control_mode);
    drive_info_pub->publish(std::move(current_state));
  }

  void TeleopDriveJoy::sendHaltCommand()
  {
    if (!current_state.autonomous_mode)
    {
      if (!sent_lock_msg)
      {
        // Initializes with zeros by default.
        auto drive_input_msg = std::make_unique<nova_interfaces::msg::DriveInputStamped>();
        drive_input_pub->publish(std::move(drive_input_msg));

        sent_lock_msg = true;
      }
    }
    else
    {
      // We want to continously publish a zero commmand to override any continuous autonomous messages

      // Initializes with zeros by default.
      auto cmd_vel_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
      cmd_vel_pub->publish(std::move(cmd_vel_msg));
    }
  }

  void TeleopDriveJoy::handleButtonCallbacks(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    // Lock and Unlock
    if (joy_msg->buttons[params_.button_unlock] && current_state.locked)
    {
      current_state.locked = false;
      RCLCPP_INFO(node_->get_logger(), "BUTTON: unlock");
    }
    else if (joy_msg->buttons[params_.button_lock] && !current_state.locked)
    {
      current_state.locked = true;
      RCLCPP_INFO(node_->get_logger(), "BUTTON: lock");
    }

    // Autonomous and Manual
    if (joy_msg->buttons[params_.button_autonomous_control] && !current_state.autonomous_mode)
    {
      current_state.autonomous_mode = true;
      RCLCPP_INFO(node_->get_logger(), "BUTTON: autonomous_control");
    }
    else if (joy_msg->buttons[params_.button_manual_control] && current_state.autonomous_mode)
    {
      current_state.autonomous_mode = false;
      RCLCPP_INFO(node_->get_logger(), "BUTTON: manual_control");
    }

    // Controller Switches
    if (joy_msg->buttons[params_.button_pivot_drive_controller] && control_mode != ControlMode::PIVOT_DRIVE)
    {
      RCLCPP_INFO(node_->get_logger(), "BUTTON: pivot_drive_mode");
      switchController(ControlMode::PIVOT_DRIVE);
    }
    else if (joy_msg->buttons[params_.button_strafe_controller] && control_mode != ControlMode::STRAFE_DRIVE)
    {
      RCLCPP_INFO(node_->get_logger(), "BUTTON: strafe_mode");
      switchController(ControlMode::STRAFE_DRIVE);
    }
    else if (joy_msg->buttons[params_.button_nova_diff_drive_controller] && control_mode != ControlMode::STRAFE_DRIVE)
    {
      RCLCPP_INFO(node_->get_logger(), "BUTTON: diff_drive_mode");
      switchController(ControlMode::DIFF_DRIVE);
    }
  }

  void TeleopDriveJoy::switchController(const ControlMode requested_control_mode)
  {
    if (requested_control_mode == control_mode)
      return;
    RCLCPP_INFO(node_->get_logger(), "Changing from %s to %s", prettyPrintMode(control_mode).c_str(), prettyPrintMode(requested_control_mode).c_str());
    std::string activate_controller = modeToController(requested_control_mode);
    std::string deactivate_controller = modeToController(control_mode);

    auto request = std::make_shared<controller_manager_msgs::srv::SwitchController::Request>();
    request->activate_controllers.emplace_back(activate_controller);
    request->deactivate_controllers.emplace_back(deactivate_controller);
    request->strictness = 2;
    request->activate_asap = true;

    switch_controller_client->async_send_request(request, [this, requested_control_mode](rclcpp::Client<controller_manager_msgs::srv::SwitchController>::SharedFuture result)
                                                 {
    if (result.get()->ok) {
      RCLCPP_INFO(node_->get_logger(), "Successfully switched to %s.", prettyPrintMode(requested_control_mode).c_str());
      control_mode = requested_control_mode;
    } else {
      RCLCPP_ERROR(node_->get_logger(), "Failed to switch to %s. Is drive.launch.py running?", prettyPrintMode(requested_control_mode).c_str());
    } });
  }

} // namespace teleop_drive_joy

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(teleop_drive_joy::TeleopDriveJoy)
