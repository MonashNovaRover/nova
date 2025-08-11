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
 * @brief Controller for a four wheel steering mobile base.
 *
 * @authors Terry Tian
 */

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>
#include <queue>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"

#include "nova_controller_common/hardware_interface_wrapper.hpp"
#include "pivot_drive_controller/kinematics.hpp"
#include "pivot_drive_controller/pivot_drive_controller.hpp"

namespace pivot_drive_controller
{

using namespace std::chrono_literals;
using namespace nova_controller_common;
using nova_drive_controller_base::Commands;
using controller_interface::interface_configuration_type;
using controller_interface::InterfaceConfiguration;
using geometry_msgs::msg::Twist;
using geometry_msgs::msg::TwistStamped;
using hardware_interface::HW_IF_POSITION;
using hardware_interface::HW_IF_VELOCITY;
using lifecycle_msgs::msg::State;

PivotDriveController::PivotDriveController() : nova_drive_controller_base::NovaDriveControllerBase()
{
}

void PivotDriveController::init_params()
{
  // Initialize parameters specific to the pivot drive controller
  param_listener_ = std::make_shared<ParamListener>(get_node());
  params_ = param_listener_->get_params();

  half_wheel_base_ = base_params_->wheel_base / 2;
  half_steering_track_ = base_params_->steering_track / 2;
  wheels_per_side_ = base_params_->left_drive_names.size();

  zero_radius_ = std::hypot(half_wheel_base_, half_steering_track_);
  RCLCPP_INFO_STREAM(get_node()->get_logger(), "zero_radius_: " << zero_radius_);

  // Solve for r = sqrt((r - half_steering_track_)^2 + half_wheel_base_^2)
  inner_radius_ = (std::pow(half_steering_track_, 2) + std::pow(half_wheel_base_, 2)) /
                  (2 * half_steering_track_);
  RCLCPP_INFO_STREAM(get_node()->get_logger(), "inner_radius_: " << inner_radius_);
}

void PivotDriveController::update_params()
{
  if (param_listener_->is_old(params_))
  {
    params_ = param_listener_->get_params();
    RCLCPP_INFO(get_node()->get_logger(), "Parameters were updated");
  }
}

Commands PivotDriveController::twist_to_commands(
  const Twist& twist_msg, bool autonomous_mode, const rclcpp::Duration& period)
{
  auto logger = get_node()->get_logger();

  double linear_input = twist_msg.linear.x;
  double angular_input = twist_msg.angular.z;
  double linear_velocity, angular_velocity;
  bool turning_left;
  double speed, turning_radius;

  // Brake if cmd_vel has timed out, override the stored command
  if (autonomous_mode)
  {
    angular_velocity = angular_input;
    linear_velocity = linear_input;
    speed = linear_velocity == 0 ? std::abs(zero_radius_ * angular_velocity) : linear_input;

    limiter_speed_.limit(speed, previous_speeds_[0], previous_speeds_[1], period.seconds());
    limiter_angular_.limit(
      angular_velocity, previous_angular_velocities_[0], previous_angular_velocities_[1],
      period.seconds());

    turning_radius = get_radius_from_velocities(linear_velocity, angular_velocity);
    turning_left = turning_radius == 0 ? angular_input > 0 : turning_radius > 0;

    const auto [max_requested_angle, left] = limit_radius_by_pivots(
      turning_radius, turning_left, half_steering_track_, half_wheel_base_, limiter_pivot_,
      previous_left_pivot_positions_, previous_right_pivot_positions_, period.seconds());

    const double requested_angular = angular_velocity;
    limiter_angular_.limit(
      angular_velocity, previous_angular_velocities_[0], previous_angular_velocities_[1],
      period.seconds());
    if (angular_velocity != requested_angular)
    {
      limit_speed_and_radius_by_angular(
        speed, turning_radius, angular_velocity, zero_radius_, inner_radius_, limiter_speed_,
        previous_speeds_, period.seconds());
    }

    const auto& prev_positions =
      left ? previous_left_pivot_positions_ : previous_right_pivot_positions_;
    double limited_angle = max_requested_angle;
    limiter_pivot_.limit(
      limited_angle, prev_positions[0], prev_positions[1], prev_positions[2],
      params_.pivot_rate_tolerance);
    if (limited_angle != max_requested_angle)
    {
      speed = 0.0;  // wait for the pivot to be within tolerance before moving
      limiter_speed_.limit(speed, previous_speeds_[0], previous_speeds_[1], period.seconds());
    }

    linear_velocity = turning_radius == 0 ? 0.0 : speed;
    angular_velocity = get_angular_from_radius_and_speed(
      turning_radius, speed, turning_left, zero_radius_, inner_radius_);
  }
  else
  {
    // Manual operation: left stick controls speed and right stick controls the pivot angle
    // Process raw angular input through a curve to calculate the turning radius
    // Prioritise keeping turning radius over speed
    turning_radius =
      angular_input == 0
        ? INFINITY
        : params_.input_curve_factor * ((1.0 / angular_input) - std::copysign(1, angular_input));
    turning_left = turning_radius == 0 ? angular_input > 0 : turning_radius > 0;

    limit_radius_by_pivots(
      turning_radius, turning_left, half_steering_track_, half_wheel_base_, limiter_pivot_,
      previous_left_pivot_positions_, previous_right_pivot_positions_, period.seconds());

    speed = linear_input * base_params_->speed.max_velocity;
    const double requested_speed = speed;
    limiter_speed_.limit(speed, previous_speeds_[0], previous_speeds_[1], period.seconds());
    if (speed != requested_speed)
    {
      RCLCPP_INFO(logger, "Speed limited to %.2f", speed);
    }
    RCLCPP_INFO(logger, "Received: Speed = %.2f, Turning radius = %f", speed, turning_radius);

    // Calculate the angular velocity based on the limited speed
    angular_velocity = get_angular_from_radius_and_speed(
      turning_radius, speed, turning_left, zero_radius_, inner_radius_);
    RCLCPP_INFO(logger, "Calculated angular velocity = %.2f", angular_velocity);

    const double requested_angular = angular_velocity;
    limiter_angular_.limit(
      angular_velocity, previous_angular_velocities_[0], previous_angular_velocities_[1],
      period.seconds());
    if (angular_velocity != requested_angular)
    {
      limit_speed_and_radius_by_angular(
        speed, turning_radius, angular_velocity, zero_radius_, inner_radius_, limiter_speed_,
        previous_speeds_, period.seconds());
    }
    linear_velocity = turning_radius == 0 ? 0 : speed;
  }

  // ######################### Send commands #############################
  const double left_angle = get_pivot_angle_from_radius(
    turning_radius, true, turning_left, half_steering_track_, half_wheel_base_);
  const double right_angle = get_pivot_angle_from_radius(
    turning_radius, false, turning_left, half_steering_track_, half_wheel_base_);
  double left_ratio = get_speed_ratio(
    turning_radius, true, half_steering_track_, half_wheel_base_, zero_radius_, inner_radius_);
  double right_ratio = get_speed_ratio(
    turning_radius, false, half_steering_track_, half_wheel_base_, zero_radius_, inner_radius_);
  const double left_speed = speed * left_ratio;
  const double right_speed = speed * right_ratio;

  // Update the previous command values for limiting
  previous_speeds_.pop_front();
  previous_speeds_.push_back(speed);
  previous_angular_velocities_.pop_front();
  previous_angular_velocities_.push_back(angular_velocity);
  previous_left_pivot_positions_.pop_front();
  previous_left_pivot_positions_.push_back(left_angle);
  previous_right_pivot_positions_.pop_front();
  previous_right_pivot_positions_.push_back(right_angle);

  RCLCPP_DEBUG(logger, "speed ratios: left = %.2f, right = %.2f", left_ratio, right_ratio);

  RCLCPP_DEBUG(
    logger, "Set drive commands: left_speed = %.2f, right_speed = %.2f", left_speed, right_speed);
  RCLCPP_DEBUG(
    logger, "Set pivot commands: left_angle = %.2f, right_angle = %.2f", left_angle, right_angle);
  RCLCPP_DEBUG(
    logger, "------------------------------------------------------------------------------------");

  return {
    .linear_velocity_x = linear_velocity,
    .linear_velocity_y = 0.0,
    .angular_velocity = angular_velocity,
    .left_drive_speeds = {left_speed, left_speed},
    .right_drive_speeds = {right_speed, right_speed},
    .left_pivot_positions = {left_angle, -left_angle},
    .right_pivot_positions = {right_angle, -right_angle},
  };
}

void PivotDriveController::reset_limiter_buffers()
{
  // Reset the previous command values for limiting
  previous_speeds_ = {0.0, 0.0};
  previous_angular_velocities_ = {0.0, 0.0};
  previous_left_pivot_positions_ = {0.0, 0.0, 0.0};
  previous_right_pivot_positions_ = {0.0, 0.0, 0.0};
}

}  // namespace pivot_drive_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
  pivot_drive_controller::PivotDriveController, nova_drive_controller_base::NovaDriveControllerBase)
