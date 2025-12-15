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

  front_left_wheel = {
    .name = "front left wheel",
    .position = {half_wheel_base_, half_steering_track_ },
    .centre_angle = std::atan(half_steering_track_ / half_wheel_base_)
  };

  RCLCPP_INFO(get_node()->get_logger(), std::format("{} created with centre_angle: {} and position: {}, {}", front_left_wheel.name, front_left_wheel.centre_angle, front_left_wheel.position.x(), front_left_wheel.position.y()).c_str());

  front_right_wheel = {
    .name = "front right wheel",
    .position = {half_wheel_base_, -half_steering_track_ },
    .centre_angle = -std::atan(half_steering_track_ / half_wheel_base_)
  };

  back_left_wheel = {
    .name = "back left wheel",
    .position = {-half_wheel_base_, half_steering_track_ },
    .centre_angle = -std::atan(half_steering_track_ / half_wheel_base_)
  };

  back_right_wheel = {
    .name = "back right wheel",
    .position = {-half_wheel_base_, -half_steering_track_ },
    .centre_angle = std::atan(half_steering_track_ / half_wheel_base_)
  };
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

  auto [flw_speed, flw_pivot_angle, flw_speed_multiplier] = calculate_wheel_speed_and_angle(front_left_wheel, linear_velocity, angular_velocity, period.seconds());
  auto [frw_speed, frw_pivot_angle, frw_speed_multiplier] = calculate_wheel_speed_and_angle(front_right_wheel, linear_velocity, angular_velocity, period.seconds());
  auto [blw_speed, blw_pivot_angle, blw_speed_multiplier] = calculate_wheel_speed_and_angle(back_left_wheel, linear_velocity, angular_velocity, period.seconds());
  auto [brw_speed, brw_pivot_angle, brw_speed_multiplier] = calculate_wheel_speed_and_angle(back_right_wheel, linear_velocity, angular_velocity, period.seconds());

  const double speed_multiplier = std::min({flw_speed_multiplier, frw_speed_multiplier, blw_speed_multiplier, brw_speed_multiplier});

  RCLCPP_DEBUG(logger, "Speed multiplied by %.2f", speed_multiplier);

  flw_speed *= speed_multiplier;
  frw_speed *= speed_multiplier;
  blw_speed *= speed_multiplier;
  brw_speed *= speed_multiplier;

  linear_velocity *= speed_multiplier;
  angular_velocity *= speed_multiplier;

  front_left_wheel.previous_angle = flw_pivot_angle;
  front_right_wheel.previous_angle = frw_pivot_angle;
  back_left_wheel.previous_angle = blw_pivot_angle;
  back_right_wheel.previous_angle = brw_pivot_angle;

  // Update the previous command values for limiting
  previous_velocities_.pop_front();
  previous_velocities_.push_back(linear_velocity);
  previous_angular_velocities_.pop_front();
  previous_angular_velocities_.push_back(angular_velocity);

  front_left_wheel.previous_pivot_positions.pop_front();
  front_left_wheel.previous_pivot_positions.push_back(flw_pivot_angle);
  front_right_wheel.previous_pivot_positions.pop_front();
  front_right_wheel.previous_pivot_positions.push_back(frw_pivot_angle);
  back_left_wheel.previous_pivot_positions.pop_front();
  back_left_wheel.previous_pivot_positions.push_back(blw_pivot_angle);
  back_right_wheel.previous_pivot_positions.pop_front();
  back_right_wheel.previous_pivot_positions.push_back(brw_pivot_angle);

  RCLCPP_INFO(
    logger, "Set drive commands: flw_speed = %.2f, frw_speed = %.2f, blw_speed = %.2f, brw_speed = %.2f", flw_speed, frw_speed, blw_speed, brw_speed);
  RCLCPP_INFO(
    logger, "Set pivot commands: flw_pivot_angle = %.2f, frw_pivot_angle = %.2f, blw_pivot_angle = %.2f, brw_pivot_angle = %.2f", flw_pivot_angle, frw_pivot_angle, blw_pivot_angle, brw_pivot_angle);
  RCLCPP_INFO(
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

std::tuple<double, double, double> HolonomicDriveController::calculate_wheel_speed_and_angle(const Wheel& wheel, const Vector2d& linear_velocity, const double angular_velocity,
  const double dt)
{
  Vector2d wheel_velocity = get_wheel_velocity(linear_velocity, angular_velocity, wheel.position);
  double pivot_angle = get_pivot_angle(wheel_velocity);
  double wheel_speed = wheel_velocity.norm();

  RCLCPP_INFO(get_node()->get_logger(), std::format("{} has requested angle {}", wheel.name, pivot_angle).c_str());

  if (params_.infinitely_rotating_pivots)
  {
    restrict_pivot_angle(pivot_angle, wheel.previous_pivot_positions[2], get_angle_between(wheel.previous_pivot_positions[1], wheel.previous_pivot_positions[2]) <= 0, params_.pivot_angle_leeway, wheel_speed);
  }
  else
  {
    restrict_pivot_angle(pivot_angle, wheel.centre_angle, get_angle_between(wheel.centre_angle, wheel.previous_pivot_positions[2]) <= 0, params_.pivot_angle_leeway, wheel_speed);
  }

  const double requested_angle = pivot_angle;

  limiter_pivot_.limit(pivot_angle, wheel.previous_pivot_positions[2], wheel.previous_pivot_positions[1], wheel.previous_pivot_positions[0], dt);

  if (params_.infinitely_rotating_pivots)
  {

  }
  else
  {
    pivot_angle = std::clamp(pivot_angle, wheel.centre_angle - M_PI_2 + params_.pivot_limit_buffer, wheel.centre_angle + M_PI_2 - params_.pivot_limit_buffer);
  }

  double max_speed_multiplier = 1;

  if (requested_angle != pivot_angle)
  {
    const double angle_deviation = std::abs(get_angle_between(requested_angle, pivot_angle));
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

  // front_right_wheel.previous_pivot_velocities = {0.0, 0.0};
  // front_left_wheel.previous_pivot_velocities = {0.0, 0.0};
  // back_right_wheel.previous_pivot_velocities = {0.0, 0.0};
  // back_left_wheel.previous_pivot_velocities = {0.0, 0.0};

  front_left_wheel.previous_pivot_positions = {0.0, 0.0, 0.0};
  front_right_wheel.previous_pivot_positions = {0.0, 0.0, 0.0};
  back_left_wheel.previous_pivot_positions = {0.0, 0.0, 0.0};
  back_right_wheel.previous_pivot_positions = {0.0, 0.0, 0.0};
}

}  // namespace holonomic_drive_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
  holonomic_drive_controller::HolonomicDriveController, controller_interface::ControllerInterface)
