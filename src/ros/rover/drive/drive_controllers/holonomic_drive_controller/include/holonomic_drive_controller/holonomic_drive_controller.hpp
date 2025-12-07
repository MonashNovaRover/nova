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
 * BRIEF: Controller for a four wheel steering mobile base using a pivot drive model.
 * A pivot drive model is a model where all wheels will be tangent towards a turning point.
 * This will cause the wheels of the vehicle to trace a circular path around the turning point.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * PLUGIN: pivot_drive_controller
 * TOPICS:
 *  - subscriber:  /cmd_vel       [geometry_msgs/msg/TwistStamped]
 *  - publisher:   ~/cmd_vel_out  [geometry_msgs/msg/TwistStamped]
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * PACKAGE:   holonomic_drive_controller
 * AUTHORS:	  Terry Tian, Jonathan Jia
 * CREATION:  2025
 * EDITED:    2025
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef HOLONOMIC_DRIVE_CONTROLLER__HOLONOMIC_DRIVE_CONTROLLER_HPP_
#define HOLONOMIC_DRIVE_CONTROLLER__HOLONOMIC_DRIVE_CONTROLLER_HPP_

#include <memory>
#include <deque>
#include <vector>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"

#include "nova_drive_controller_base/nova_drive_controller_base.hpp"
#include "nova_drive_controller_base/odometry.hpp"
#include "pivot_drive_controller_parameters.hpp"

namespace holonomic_drive_controller
{

class HolonomicDriveController : public nova_drive_controller_base::NovaDriveControllerBase
{
public:
  HolonomicDriveController();

protected:
  void init_params() override;
  void update_params() override;
  void reset_limiter_buffers() override;
  nova_drive_controller_base::Commands twist_to_commands(
    const geometry_msgs::msg::Twist& twist_msg, bool autonomous_mode,
    const rclcpp::Duration& period) override;

  // Parameters from ROS for pivot_drive_controller
  std::shared_ptr<ParamListener> param_listener_;
  Params params_;

  // Radius of the circle the rover makes with its wheels when turning on the spot
  double zero_radius_;
  // Turning radius at which the radius of the circle that the wheel on the side of the turn
  // makes is equal. Any turning radius less than this will cause the radius of the circle that
  // the wheel on the side of the turn makes to be larger than the turning radius.
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

}  // namespace holonomic_drive_controller

#endif  // HOLONOMIC_DRIVE_CONTROLLER__HOLONOMIC_DRIVE_CONTROLLER_HPP_
