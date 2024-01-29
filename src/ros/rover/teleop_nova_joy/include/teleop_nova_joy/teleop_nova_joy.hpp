/**
Software License Agreement (BSD)

\authors   Mike Purvis <mpurvis@clearpathrobotics.com>
\copyright Copyright (c) 2014, Clearpath Robotics, Inc., All rights reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:
 * Redistributions of source code must retain the above copyright notice, this list of conditions and the
   following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
   following disclaimer in the documentation and/or other materials provided with the distribution.
 * Neither the name of Clearpath Robotics nor the names of its contributors may be used to endorse or promote
   products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WAR-
RANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, IN-
DIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef TELEOP_NOVA_JOY_TELEOP_NOVA_JOY_H
#define TELEOP_NOVA_JOY_TELEOP_NOVA_JOY_H

#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/joy.hpp>
#include <geometry_msgs/msg/twist_stamped.hpp>
#include <core/msg/drive_input_stamped.hpp>
#include <core/msg/drive_info.hpp>
#include <controller_manager_msgs/srv/switch_controller.hpp>


#include "teleop_nova_joy_parameters.hpp"

namespace teleop_nova_joy
{
  class TeleopNovaJoy
  {
  public:
    explicit TeleopNovaJoy(const rclcpp::NodeOptions& options);

    void joyCallback(const sensor_msgs::msg::Joy::SharedPtr joy_msg);
    void sendCmdVelMsg(const sensor_msgs::msg::Joy::SharedPtr joy_msg, const std::string& which_map);
    double getVal(const sensor_msgs::msg::Joy::SharedPtr joy_msg, const std::map<std::string, int64_t>& axis_map,
                const std::map<std::string, double>& scale_map, const std::string& fieldname);

    rclcpp::node_interfaces::NodeBaseInterface::SharedPtr get_node_base_interface();  // NOLINT

  private:

    const rclcpp::Node::SharedPtr node_;
    std::shared_ptr<teleop_nova_joy::ParamListener> param_listener_;
    teleop_nova_joy::Params params_;

    rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr cmd_vel_pub;
    rclcpp::Publisher<core::msg::DriveInputStamped>::SharedPtr drive_input_pub;
    rclcpp::Publisher<core::msg::DriveInfo>::SharedPtr drive_info_pub;
    rclcpp::Client<controller_manager_msgs::srv::SwitchController>::SharedPtr switch_controller_client;
    rclcpp::Subscription<sensor_msgs::msg::Joy>::SharedPtr joy_sub;
    rclcpp::Client<rcl_interfaces::srv::SetParameters>::SharedPtr parameters_client;

    bool sent_lock_msg;
    core::msg::DriveInfo current_state;
    core::msg::DriveInfo previous_state;

    bool speed_change_button_pressed; 

    std::map<std::string, int64_t> axis_linear_map{
      {"x", 1L},
      {"y", -1L},
      {"z", -1L},
    };

    std::map<std::string, std::map<std::string, double>> scale_linear_map;

    std::map<std::string, int64_t> axis_angular_map;
    std::map<std::string, std::map<std::string, double>> scale_angular_map;

  };
}  // namespace teleop_nova_joy

#endif  // TELEOP_NOVA_JOY_TELEOP_NOVA_JOY_H
