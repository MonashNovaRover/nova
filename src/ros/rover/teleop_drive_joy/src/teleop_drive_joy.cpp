/**
Software License Agreement (BSD)

\authors   Mike Purvis <mpurvis@clearpathrobotics.com>
\copyright Copyright (c) 2014, Clearpath Robotics, Inc., All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, node_ list of conditions and the
   following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, node_ list of conditions and the
   following disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of Clearpath Robotics nor the names of its contributors may be used to endorse or promote
   products derived from node_ software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WAR-
RANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, IN-
DIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
    drive_input_pub = node_->create_publisher<drive_interfaces::msg::DriveInputStamped>(DEFAULT_OUTPUT_TOPIC, 50);
    cmd_vel_pub = node_->create_publisher<geometry_msgs::msg::TwistStamped>(DEFAULT_OUTPUT_TOPIC_TWIST, 50);
    drive_info_pub = node_->create_publisher<drive_interfaces::msg::DriveInfo>(DEFAULT_OUTPUT_TOPIC_INFO, 50);

    joy_sub = node_->create_subscription<sensor_msgs::msg::Joy>(DEFAULT_INPUT_TOPIC, rclcpp::QoS(10), std::bind(&TeleopDriveJoy::joyCallback, this, _1));

    switch_controller_client = node_->create_client<controller_manager_msgs::srv::SwitchController>("/controller_manager/switch_controller");

    param_listener_ = std::make_shared<ParamListener>(node_);
    params_ = param_listener_->get_params();

    if (param_listener_->is_old(params_))
    {
      params_ = param_listener_->get_params();
    }

    parameters_client = node_->create_client<rcl_interfaces::srv::SetParameters>("/pivot_drive_controller/set_parameters");
  }

  double TeleopDriveJoy::getVal(const sensor_msgs::msg::Joy::SharedPtr joy_msg, const std::map<std::string, int64_t> &axis_map,
                                const std::map<std::string, double> &scale_map, const std::string &fieldname)
  {
    if (axis_map.find(fieldname) == axis_map.end() ||
        axis_map.at(fieldname) == -1L ||
        scale_map.find(fieldname) == scale_map.end() ||
        static_cast<int>(joy_msg->axes.size()) <= axis_map.at(fieldname))
    {
      return 0.0;
    }

    return joy_msg->axes[axis_map.at(fieldname)] * scale_map.at(fieldname);
  }

  void TeleopDriveJoy::sendDriveCommand(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {

    auto controller_params = params_.controllers_map.at(modeToController(control_mode));

    double angular = joy_msg->axes[controller_params.axis_angular.z] * controller_params.scale_angular.z;
    double linear = joy_msg->axes[controller_params.axis_linear.x] * controller_params.scale_linear.x;

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
      auto drive_input_msg = std::make_unique<drive_interfaces::msg::DriveInputStamped>();

      drive_input_msg->drive_input.radius = angular == 0 ? INFINITY : (1.0 / pow(abs(angular), 2)) - 1;
      drive_input_msg->drive_input.direction = angular > 0 ? -1 : angular < 0 ? 1
                                                                              : 0;
      drive_input_msg->drive_input.speed = linear;
      drive_input_msg->drive_input.mode = previous_state.drive_mode;
      drive_input_msg->header.stamp = node_->now();

      drive_input_pub->publish(std::move(drive_input_msg));
    }

    sent_lock_msg = false;
    previous_state = current_state;
    drive_info_pub->publish(std::move(current_state));
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
    if (joy_msg->buttons[params_.button_pivot_drive_controller] && current_state.drive_mode != drive_interfaces::msg::DriveInput::PIVOT)
    {
      RCLCPP_INFO(node_->get_logger(), "BUTTON: pivot_drive_mode");
      switchController(ControlMode::PIVOT_DRIVE);
    }
    else if (joy_msg->buttons[params_.button_strafe_controller] && current_state.drive_mode != drive_interfaces::msg::DriveInput::STRAFE)
    {
      RCLCPP_INFO(node_->get_logger(), "BUTTON: strafe_mode");
      switchController(ControlMode::STRAFE_DRIVE);
    }
    else if (joy_msg->buttons[params_.button_nova_diff_drive_controller] && current_state.drive_mode != drive_interfaces::msg::DriveInput::TANK)
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

  void TeleopDriveJoy::joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {

    handleButtonCallbacks(joy_msg);

    if (!current_state.locked)
    {
      sendDriveCommand(joy_msg);
    }
    else
    {
      // When lock button is pressed, immediately send a single no-motion command
      // in order to stop the rover.

      if (!current_state.autonomous_mode)
      {
        if (!sent_lock_msg)
        {
          // Initializes with zeros by default.
          auto drive_input_msg = std::make_unique<drive_interfaces::msg::DriveInputStamped>();
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
  }

} // namespace teleop_drive_joy

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(teleop_drive_joy::TeleopDriveJoy)
