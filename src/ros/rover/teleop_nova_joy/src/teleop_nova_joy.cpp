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

#include "teleop_nova_joy/teleop_nova_joy.hpp"


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
namespace teleop_nova_joy
{

  rclcpp::node_interfaces::NodeBaseInterface::SharedPtr TeleopNovaJoy::get_node_base_interface()
  {
    return node_->get_node_base_interface();
  }

  /**
   * Constructs TeleopNovaJoy.
   */
  TeleopNovaJoy::TeleopNovaJoy(const rclcpp::NodeOptions& options) : node_{ std::make_shared<rclcpp::Node>("teleop_nova_joy_node_", options)}
  {
    drive_input_pub = node_->create_publisher<core::msg::DriveInputStamped>(DEFAULT_OUTPUT_TOPIC, 50);
    cmd_vel_pub = node_->create_publisher<geometry_msgs::msg::TwistStamped>(DEFAULT_OUTPUT_TOPIC_TWIST, 50);
    drive_info_pub = node_->create_publisher<core::msg::DriveInfo>(DEFAULT_OUTPUT_TOPIC_INFO, 50);
    
    joy_sub = node_->create_subscription<sensor_msgs::msg::Joy>(DEFAULT_INPUT_TOPIC, rclcpp::QoS(10), std::bind(&TeleopNovaJoy::joyCallback, this, _1));

    switch_controller_client = node_->create_client<controller_manager_msgs::srv::SwitchController>("/controller_manager/switch_controller");

    param_listener_ = std::make_shared<ParamListener>(node_);
    params_ = param_listener_->get_params();

    if (param_listener_->is_old(params_)) {
      params_ = param_listener_->get_params();
    }

    parameters_client = node_->create_client<rcl_interfaces::srv::SetParameters>("/pivot_drive_controller/set_parameters");
  }

  double TeleopNovaJoy::getVal(const sensor_msgs::msg::Joy::SharedPtr joy_msg, const std::map<std::string, int64_t>& axis_map,
                const std::map<std::string, double>& scale_map, const std::string& fieldname)
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

  void TeleopNovaJoy::sendCmdVelMsg(const sensor_msgs::msg::Joy::SharedPtr joy_msg,
                                           const std::string& which_map)
  {
    //RCLCPP_INFO(node_->get_logger(), "sending cmd_vel msg...");

    // Initializes with zeros by default.

    auto request = std::make_shared<controller_manager_msgs::srv::SwitchController::Request>();

    auto modeToController = [](const unsigned char mode) -> std::string {
      switch (mode) {
        case core::msg::DriveInput::STRAFE:
          return "strafe_controller";
        case core::msg::DriveInput::PIVOT:
          return "pivot_drive_controller";
        case core::msg::DriveInput::DIFF:
          return "nova_diff_drive_controller";
      }
    };

    auto setControllerControlType = [this, modeToController](const unsigned char mode, bool enable_twist_cmd) -> void {
      RCLCPP_INFO(node_->get_logger(), "Setting controller's enable_twist_cmd to: %s", enable_twist_cmd ? "true" : "false");
      auto request = std::make_shared<rcl_interfaces::srv::SetParameters::Request>();

      auto parameter = rcl_interfaces::msg::Parameter();

      parameter.name = "enable_twist_cmd";
      parameter.value.type = 1;
      parameter.value.bool_value = enable_twist_cmd;

      request->parameters.push_back(parameter);

      while (!parameters_client->wait_for_service(1s)) {
        if (!rclcpp::ok()) {
          RCLCPP_ERROR(node_->get_logger(), "Interrupted while waiting for the parameter service. Exiting.");
          rclcpp::shutdown();
        }
        RCLCPP_INFO(node_->get_logger(), "Parameter service not available, waiting again...");
      }

      auto result = parameters_client->async_send_request(request);
      rclcpp::Parameter enable_twist_cmd_param = rclcpp::Parameter("enable_twist_cmd", enable_twist_cmd);
    };

    if (current_state.mode != previous_state.mode)
    {
      RCLCPP_INFO(node_->get_logger(), "Changing from %s to %s", modeToController(previous_state.mode).c_str(), modeToController(current_state.mode).c_str());
      std::string activate_controller = modeToController(current_state.mode);
      std::string deactivate_controller = modeToController(previous_state.mode);

      request->activate_controllers.emplace_back(activate_controller);
      request->deactivate_controllers.emplace_back(deactivate_controller);
      request->strictness = 2;
      //request->start_asap = false;
      builtin_interfaces::msg::Duration duration;
      duration.sec = 0.0;
      duration.nanosec = 0.0;
      request->timeout = duration;
      
      while (!switch_controller_client->wait_for_service(1s))
      {
        if (!rclcpp::ok())
        {
          RCLCPP_ERROR(node_->get_logger(), "Interrupted while waiting for the service. Exiting.");
        }
        RCLCPP_INFO(rclcpp::get_logger("rclcpp"), "service not available, waiting again...");
      }

      auto future = switch_controller_client->async_send_request(request);

      if (future.wait_for(0s) != std::future_status::ready)
      {
        RCLCPP_ERROR(node_->get_logger(), "service call failed :(");
      }
    }

    double angular = joy_msg->axes[params_.axis_angular.yaw] * params_.scale_angular.yaw;
    double linear = joy_msg->axes[params_.axis_linear.x] * params_.scale_linear.x;

    if (current_state.autonomous_mode)
    {
      if (!previous_state.autonomous_mode) setControllerControlType(core::msg::DriveInput::PIVOT, true);
     
      auto cmd_vel_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
      cmd_vel_msg->twist.angular.z = angular;
      cmd_vel_msg->twist.linear.x = linear;
      cmd_vel_msg->header.stamp = node_->now();

      cmd_vel_pub->publish(std::move(cmd_vel_msg));
    }
    else
    {
      if (previous_state.autonomous_mode) setControllerControlType(core::msg::DriveInput::PIVOT, false);

      auto drive_input_msg = std::make_unique<core::msg::DriveInputStamped>();

      drive_input_msg->drive_input.radius = angular == 0 ? INFINITY : (1.0 / pow(abs(angular), 2)) - 1;
      drive_input_msg->drive_input.direction = angular > 0 ? -1 : angular < 0 ? 1 : 0;
      drive_input_msg->drive_input.speed = linear;
      drive_input_msg->drive_input.mode = previous_state.mode;
      drive_input_msg->header.stamp = node_->now();

      drive_input_pub->publish(std::move(drive_input_msg));
    }

    sent_lock_msg = false;
    previous_state = current_state;
    drive_info_pub->publish(std::move(current_state));
  }

  void TeleopNovaJoy::joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg)
  {
    /*
    if (enable_turbo_button >= 0 &&
        static_cast<int>(joy_msg->buttons.size()) > enable_turbo_button &&
        joy_msg->buttons[enable_turbo_button])
    {
      sendCmdVelMsg(joy_msg, "turbo");
    }
    else if (!require_enable_button ||
       (static_cast<int>(joy_msg->buttons.size()) > enable_button &&
             joy_msg->buttons[enable_button]))
    {
      sendCmdVelMsg(joy_msg, "normal");
    }
    */

    if (joy_msg->buttons[params_.button_unlock])
    {
      current_state.locked = false;
      RCLCPP_INFO(node_->get_logger(), "BUTTON: unlock");
    } 
    else if (joy_msg->buttons[params_.button_lock])
    {
      current_state.locked = true;
      RCLCPP_INFO(node_->get_logger(), "BUTTON: lock");
    }
    if (joy_msg->buttons[params_.button_autonomous_control])
    {
      current_state.autonomous_mode = true;
      RCLCPP_INFO(node_->get_logger(), "BUTTON: autonomous_control");
    }
    else if (joy_msg->buttons[params_.button_manual_control])
    {
      current_state.autonomous_mode = false;
      RCLCPP_INFO(node_->get_logger(), "BUTTON: manual_control");
    }
    if (joy_msg->buttons[params_.button_strafe_mode])
    {
      current_state.mode = core::msg::DriveInput::STRAFE;
      RCLCPP_INFO(node_->get_logger(), "BUTTON: strafe_mode");
    }
    else if (joy_msg->buttons[params_.button_diff_drive_mode])
    {
      current_state.mode = core::msg::DriveInput::DIFF;
      RCLCPP_INFO(node_->get_logger(), "BUTTON: diff_drive_mode");
    }
    else if (joy_msg->buttons[params_.button_pivot_drive_mode])
    {
      current_state.mode = core::msg::DriveInput::PIVOT;
      RCLCPP_INFO(node_->get_logger(), "BUTTON: pivot_drive_mode");
    }
    else if (joy_msg->axes[params_.axis_speed_change_coarse] && !speed_change_button_pressed)
    {
      params_.scale_linear.x += params_.speed_change_coarse_val * joy_msg->axes[params_.axis_speed_change_coarse];
      RCLCPP_INFO(node_->get_logger(), "Current max speed: %f", params_.scale_linear.x);

      speed_change_button_pressed = true;
    }
    else if (joy_msg->axes[params_.axis_speed_change_fine] && !speed_change_button_pressed)
    {
      params_.scale_linear.x += params_.speed_change_fine_val * joy_msg->axes[params_.axis_speed_change_fine] * (-1); //-1 value for reversing direction of Joy axes
      RCLCPP_INFO(node_->get_logger(), "Current max speed: %f", params_.scale_linear.x);

      speed_change_button_pressed = true;
    }

    //reset speed change axes state
    if (!joy_msg->axes[params_.axis_speed_change_fine] && !joy_msg->axes[params_.axis_speed_change_coarse])
    {
      speed_change_button_pressed = false;
    }

    //FOR TESTING PURPOSES: try TwistStamped control with operators 
    //if (!locked && manual_teleop)
    if (!current_state.locked)
    {
      sendCmdVelMsg(joy_msg, "normal");
    }
    else
    {
      // When lock button is pressed, immediately send a single no-motion command
      // in order to stop the robot.

      if (!current_state.autonomous_mode)
      {
        if (!sent_lock_msg)
        {
          // Initializes with zeros by default.
          auto drive_input_msg = std::make_unique<core::msg::DriveInputStamped>();
          drive_input_pub->publish(std::move(drive_input_msg));

          sent_lock_msg = true;
        }
      }
      else
      {
        //We want to continously publish a zero commmand to override any continuous autonomous messages
        
        // Initializes with zeros by default.
        auto cmd_vel_msg = std::make_unique<geometry_msgs::msg::TwistStamped>();
        cmd_vel_pub->publish(std::move(cmd_vel_msg));
      }
    }
  }

}  // namespace teleop_nova_joy

#include "rclcpp_components/register_node_macro.hpp"
RCLCPP_COMPONENTS_REGISTER_NODE(teleop_nova_joy::TeleopNovaJoy)
