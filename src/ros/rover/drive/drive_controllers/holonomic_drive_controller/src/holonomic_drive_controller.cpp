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
 * PACKAGE: holonomic_drive_controller
 * AUTHORS:	Terry Tian, Jonathan Jia
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <algorithm>
#include <cmath>
#include <memory>
#include <string>
#include <vector>
#include <Eigen/Dense>

#include "controller_interface/controller_interface.hpp"
#include "rclcpp/rclcpp.hpp"

#include "holonomic_drive_controller/kinematics.hpp"
#include "holonomic_drive_controller/holonomic_drive_controller.hpp"

namespace holonomic_drive_controller
{

using geometry_msgs::msg::Twist;
using nova_drive_controller_base::Commands;
using Eigen::Vector2d;

HolonomicDriveController::HolonomicDriveController()
  : nova_drive_controller_base::NovaDriveControllerBase()
{
}

void HolonomicDriveController::init_params()
{
  // Initialize parameters specific to the pivot drive controller
  param_listener_ = std::make_shared<ParamListener>(get_node());
  params_ = param_listener_->get_params();

  half_wheel_base_ = base_params_->wheel_base / 2;
  half_steering_track_ = base_params_->steering_track / 2;
  zero_radius_ = std::hypot(half_wheel_base_, half_steering_track_);
  RCLCPP_INFO_STREAM(get_node()->get_logger(), "zero_radius_: " << zero_radius_);

  // Let r = the turning radius, x = half_steering_track_, y = half_wheel_base_.
  // sqrt((r - x)^2 + y^2) is the radius of the circle that the wheel on the side
  // of the turn makes.
  // Solve for r = sqrt((r - x)^2 + y^2)
  inner_radius_ = (std::pow(half_steering_track_, 2) + std::pow(half_wheel_base_, 2)) /
                  (2 * half_steering_track_);
  RCLCPP_INFO_STREAM(get_node()->get_logger(), "inner_radius_: " << inner_radius_);
}

void HolonomicDriveController::update_params()
{
  if (param_listener_->is_old(params_))
  {
    params_ = param_listener_->get_params();
    RCLCPP_INFO(get_node()->get_logger(), "Parameters were updated");
  }
}

Commands HolonomicDriveController::twist_to_commands(
  const Twist& twist_msg, bool autonomous_mode, const rclcpp::Duration& period)
{
  auto logger = get_node()->get_logger();

  Vector2d linear_velocity {twist_msg.linear.x, twist_msg.linear.y};
  double angular_velocity = twist_msg.angular.z;

  limiter_drive_velocity_.limit(linear_velocity, previous_velocities_[1], previous_velocities_[0], period.seconds());

  limiter_angular_.limit(
  angular_velocity, previous_angular_velocities_[1], previous_angular_velocities_[0],
  period.seconds());

  auto [flw_speed, flw_pivot_angle, flw_speed_multiplier] = calculate_wheel_speed_and_angle("front left wheel", {half_steering_track_, half_wheel_base_}, M_PI_4, previous_front_left_pivot_positions_, linear_velocity, angular_velocity, period.seconds());
  auto [frw_speed, frw_pivot_angle, frw_speed_multiplier] = calculate_wheel_speed_and_angle("front right wheel", {half_steering_track_, -half_wheel_base_}, -M_PI_4, previous_front_right_pivot_positions_, linear_velocity, angular_velocity, period.seconds());
  auto [blw_speed, blw_pivot_angle, blw_speed_multiplier] = calculate_wheel_speed_and_angle("back left wheel", {-half_steering_track_, half_wheel_base_}, -M_PI_4, previous_back_left_pivot_positions_, linear_velocity, angular_velocity, period.seconds());
  auto [brw_speed, brw_pivot_angle, brw_speed_multiplier] = calculate_wheel_speed_and_angle("back right wheel", {-half_steering_track_, -half_wheel_base_}, M_PI_4, previous_back_right_pivot_positions_, linear_velocity, angular_velocity, period.seconds());

  const double speed_multiplier = std::min({flw_speed_multiplier, frw_speed_multiplier, blw_speed_multiplier, brw_speed_multiplier});

  RCLCPP_DEBUG(logger, "Speed multiplied by %.2f", speed_multiplier);

  flw_speed *= speed_multiplier;
  frw_speed *= speed_multiplier;
  blw_speed *= speed_multiplier;
  brw_speed *= speed_multiplier;

  linear_velocity *= speed_multiplier;
  angular_velocity *= speed_multiplier;

  // Update the previous command values for limiting
  previous_velocities_.pop_front();
  previous_velocities_.push_back(linear_velocity);
  previous_angular_velocities_.pop_front();
  previous_angular_velocities_.push_back(angular_velocity);
  previous_front_left_pivot_positions_.pop_front();
  previous_front_left_pivot_positions_.push_back(flw_pivot_angle);
  previous_front_right_pivot_positions_.pop_front();
  previous_front_right_pivot_positions_.push_back(frw_pivot_angle);
  previous_back_left_pivot_positions_.pop_front();
  previous_back_left_pivot_positions_.push_back(blw_pivot_angle);
  previous_back_right_pivot_positions_.pop_front();
  previous_back_right_pivot_positions_.push_back(brw_pivot_angle);

  RCLCPP_DEBUG(
    logger, "Set drive commands: flw_speed = %.2f, frw_speed = %.2f, blw_speed = %.2f, brw_speed = %.2f", flw_speed, frw_speed, blw_speed, brw_speed);
  RCLCPP_DEBUG(
    logger, "Set pivot commands: flw_pivot_angle = %.2f, frw_pivot_angle = %.2f, blw_pivot_angle = %.2f, brw_pivot_angle = %.2f", flw_pivot_angle, frw_pivot_angle, blw_pivot_angle, brw_pivot_angle);
  RCLCPP_DEBUG(
    logger, "------------------------------------------------------------------------------------");

  return {
    .linear_velocity_x = linear_velocity.x(),
    .linear_velocity_y = linear_velocity.y(),
    .angular_velocity = angular_velocity,
    .left_drive_speeds = std::vector<double>{flw_speed, blw_speed},
    .right_drive_speeds = std::vector<double>{frw_speed, brw_speed},
    .left_pivot_positions = {flw_pivot_angle, blw_pivot_angle},
    .right_pivot_positions = {frw_pivot_angle, brw_pivot_angle},
  };
}

std::tuple<double, double, double> HolonomicDriveController::calculate_wheel_speed_and_angle(
  std::string_view wheel_name, const Vector2d& wheel_position, const double& centre_angle,
  const std::deque<double>& previous_pivot_positions, const Vector2d& linear_velocity, const double& angular_velocity,
  const double dt)
{
  Vector2d wheel_velocity = get_wheel_velocity(linear_velocity, angular_velocity, wheel_position);
  double pivot_angle = get_pivot_angle(wheel_velocity);
  double wheel_speed = wheel_velocity.norm();

  if (params_.infinitely_rotating_pivots)
  {
    restrict_pivot_angle(pivot_angle, previous_pivot_positions[2], wheel_speed);
  }
  else
  {
    restrict_pivot_angle(pivot_angle, centre_angle, wheel_speed);
    // const double lowest_angle = centre_angle - M_PI_2;
    // const double highest_angle = centre_angle + M_PI_2;
    //
    // // pivot_angle less than centre_angle - pi/2
    // if (pivot_angle < (lowest_angle - params_.pivot_angle_leeway))
    // {
    //   pivot_angle += M_PI;
    //   wheel_speed *= -1;
    // }
    //
    // // pivot_angle approximately equals centre_angle - pi/2
    // else if (pivot_angle <= (lowest_angle + params_.pivot_angle_leeway))
    // {
    //   if (get_angle_between(centre_angle, previous_pivot_positions[2]) > 0)
    //   {
    //     pivot_angle = highest_angle;
    //     wheel_speed *= -1;
    //   }
    //   else
    //   {
    //     pivot_angle = lowest_angle;
    //   }
    // }
    //
    // // pivot_angle greater than centre_angle + pi/2
    // else if (pivot_angle > (highest_angle + params_.pivot_angle_leeway))
    // {
    //   pivot_angle -= M_PI;
    //   wheel_speed *= -1;
    // }
    //
    // // pivot_angle approximately equals centre_angle + pi/2
    // else if (pivot_angle >= (highest_angle - params_.pivot_angle_leeway))
    // {
    //   if (get_angle_between(centre_angle, previous_pivot_positions[2]) < 0)
    //   {
    //     pivot_angle = lowest_angle;
    //     wheel_speed *= -1;
    //   }
    //   else
    //   {
    //     pivot_angle = highest_angle;
    //   }
    // }
  }

  const double requested_angle = pivot_angle;

  limiter_pivot_.limit(pivot_angle, previous_pivot_positions[2], previous_pivot_positions[1], previous_pivot_positions[0], dt);

  if (params_.infinitely_rotating_pivots)
  {

  }
  else
  {
    pivot_angle = std::clamp(pivot_angle, centre_angle - M_PI_2 + params_.pivot_limit_buffer, centre_angle + M_PI_2 - params_.pivot_limit_buffer);
  }

  double max_speed_multiplier = 1;

  if (requested_angle != pivot_angle)
  {
    const double angle_deviation = get_angle_between(requested_angle, pivot_angle);
    wheel_speed *= std::cos(angle_deviation);
    max_speed_multiplier = std::pow(1 - std::abs(std::sin(angle_deviation)), params_.speed_multiplier_exponent);
  }

  return {wheel_speed, pivot_angle, max_speed_multiplier};
}

void HolonomicDriveController::reset_limiter_buffers()
{
  // Reset the previous command values for limiting
  previous_velocities_ = {{0.0, 0.0}, {0.0, 0.0}};
  previous_angular_velocities_ = {0.0, 0.0};
  previous_front_left_pivot_positions_ = {0.0, 0.0, 0.0};
  previous_front_right_pivot_positions_ = {0.0, 0.0, 0.0};
  previous_back_left_pivot_positions_ = {0.0, 0.0, 0.0};
  previous_back_right_pivot_positions_ = {0.0, 0.0, 0.0};
}

}  // namespace holonomic_drive_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
  holonomic_drive_controller::HolonomicDriveController, controller_interface::ControllerInterface)
