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
#include <Eigen/Dense>

#include "geometry_msgs/msg/twist.hpp"
#include "rclcpp/rclcpp.hpp"

#include "nova_drive_controller_base/nova_drive_controller_base.hpp"
#include "nova_drive_controller_base/odometry.hpp"
#include "holonomic_drive_controller_parameters.hpp"

namespace holonomic_drive_controller
{

struct Wheel
{
  std::string name;
  Eigen::Vector2d position;
  // pivot_angle_limits[0] is the lower limit, pivot_angle_limits[1] is the upper limit (in radians)
  std::vector<double> pivot_angle_limits;
  std::deque<double> previous_pivot_positions {0.0, 0.0, 0.0};
};

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

  std::tuple<double, double, double> calculate_wheel_speed_and_angle(
    const Wheel& wheel, const Eigen::Vector2d& linear_velocity,
    const double angular_velocity, const double dt);

  // Parameters from ROS for holonomic_drive_controller
  std::shared_ptr<ParamListener> param_listener_;
  Params params_;

  double half_wheel_base_;
  double half_steering_track_;

  Wheel front_left_wheel;
  Wheel front_right_wheel;
  Wheel back_left_wheel;
  Wheel back_right_wheel;

  std::deque<Eigen::Vector2d> previous_velocities_;    // last two speed commands
  std::deque<double> previous_angular_velocities_;     // last two angular velocity commands
};

}  // namespace holonomic_drive_controller

#endif  // HOLONOMIC_DRIVE_CONTROLLER__HOLONOMIC_DRIVE_CONTROLLER_HPP_
