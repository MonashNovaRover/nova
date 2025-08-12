// Copyright (c) 2025 Monash Nova Rover
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Monash Nova Rover Team
 * 
 * BRIEF: Controller that pivots the wheels to strafe sideways.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * PLUGIN: strafe_drive_controller
 * TOPICS:
 *  - subscriber:  /cmd_vel       [geometry_msgs/msg/TwistStamped]
 *  - publisher:   ~/cmd_vel_out  [geometry_msgs/msg/TwistStamped]
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * PACKAGE:   strafe_drive_controller
 * AUTHORS:	  Terry Tian
 * CREATION:  2025
 * EDITED:    2025
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef STRAFE_DRIVE_CONTROLLER__STRAFE_DRIVE_CONTROLLER_HPP_
#define STRAFE_DRIVE_CONTROLLER__STRAFE_DRIVE_CONTROLLER_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include <tuple>

#include "hardware_interface/handle.hpp"
#include "controller_interface/controller_interface.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "std_srvs/srv/empty.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_box.h"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"

#include "nova_controller_common/speed_limiter.hpp"
#include "nova_controller_common/hardware_interface_wrapper.hpp"
#include "nova_drive_controller_base/nova_drive_controller_base.hpp"
#include "nova_drive_controller_base/odometry.hpp"

namespace strafe_drive_controller
{

class StrafeDriveController : public nova_drive_controller_base::NovaDriveControllerBase
{
public:
  StrafeDriveController();

protected:
  void init_params() override;
  void update_params() override;
  void reset_limiter_buffers() override;
  nova_drive_controller_base::Commands twist_to_commands(
    const geometry_msgs::msg::Twist& twist_msg, bool autonomous_mode,
    const rclcpp::Duration& period) override;

  std::deque<double> previous_speeds_;  // last two speed commands
};

}  // namespace strafe_drive_controller

#endif  // STRAFE_DRIVE_CONTROLLER__STRAFE_DRIVE_CONTROLLER_HPP_
