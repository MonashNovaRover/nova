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
 * @brief Controller for a four wheel steering rover.
 * @authors Terry Tian
 */

#ifndef PIVOT_DRIVE_CONTROLLER__PIVOT_DRIVE_CONTROLLER_HPP_
#define PIVOT_DRIVE_CONTROLLER__PIVOT_DRIVE_CONTROLLER_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <deque>
#include <string>
#include <vector>
#include <utility>

#include "controller_interface/controller_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "std_srvs/srv/empty.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"

#include "nova_controller_common/speed_limiter.hpp"
#include "nova_controller_common/position_limiter.hpp"
#include "nova_controller_common/hardware_interface_wrapper.hpp"
#include "nova_drive_controller_base/odometry.hpp"
#include "pivot_drive_controller_parameters.hpp"

namespace pivot_drive_controller
{

class PivotDriveController
  : public nova_drive_controller_base::NovaDriveControllerBase<PivotDriveController>
{
public:
  PivotDriveController();

protected:
  void init_params() override;
  void update_params() override;
  void update_limiter_buffers(const Commands& cmds) override;
  void reset_limiter_buffers() override;
  Commands twist_to_commands(
    const geometry_msgs::msg::Twist& twist_msg, bool autonomous_mode) const override;

  // Parameters from ROS for pivot_drive_controller
  std::shared_ptr<ParamListener> param_listener_;
  Params params_;

  // Radius of the circle the rover makes with its wheels when turning on the spot
  double zero_radius_;
  // Turning radius at which the circle that the wheel to the side of the turn makes has the
  // same radius. Any turning radius less than this will cause the circle that the wheel
  // to the side of the turn makes to have a larger radius than the turning radius
  double inner_radius_;
  // Offset angle for the pivot joints, used to calculate the pivot angles
  double offset_angle_;
  double half_wheel_base_;
  double half_steering_track_;

  std::deque<double> previous_speeds_;                 // last two speed commands
  std::deque<double> previous_angular_velocities_;     // last two angular velocity commands
  std::deque<double> previous_left_pivot_positions_;   // last three left pivot position commands
  std::deque<double> previous_right_pivot_positions_;  // last three right pivot position commands
};

}  // namespace pivot_drive_controller

#endif  // PIVOT_DRIVE_CONTROLLER__PIVOT_DRIVE_CONTROLLER_HPP_
