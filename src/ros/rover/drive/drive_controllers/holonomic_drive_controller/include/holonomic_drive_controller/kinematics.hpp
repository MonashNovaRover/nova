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
 * @brief Kinematic functions for a pivot drive model.
 * A pivot drive model is a model where all wheels will be tangent towards a turning point.
 * This will cause the wheels of the vehicle to trace a circular path around the turning point.
 * 
 * @authors Terry Tian
 * @date Created 2025
 */

#ifndef HOLONOMIC_DRIVE_CONTROLLER__KINEMATICS_HPP_
#define HOLONOMIC_DRIVE_CONTROLLER__KINEMATICS_HPP_

#include <cmath>
#include <deque>
#include <algorithm>
#include <tuple>
#include <Eigen/Dense>
#include <fmt/core.h>

#include "nova_controller_common/speed_limiter.hpp"
#include "nova_controller_common/position_limiter.hpp"

namespace holonomic_drive_controller
{

Eigen::Vector2d get_wheel_velocity(const Eigen::Vector2d& linear_velocity, const double angular_velocity, const Eigen::Vector2d& wheel_position)
{
  return {linear_velocity.x() - angular_velocity * wheel_position.y(), linear_velocity.y() + angular_velocity * wheel_position.x()};
}

double get_pivot_angle(const Eigen::Vector2d& wheel_velocity)
{
  if (wheel_velocity.norm() == 0)
  {
    return 0;
  }
  return std::atan2(wheel_velocity.y(), wheel_velocity.x());
}

void restrict_pivot_angle(double& pivot_angle, const double centre_angle, const bool prefer_clockwise_rotation, const double epsilon, double& wheel_speed)
{
  const double lowest_angle = centre_angle - M_PI_2;
  const double highest_angle = centre_angle + M_PI_2;

  // pivot_angle less than centre_angle - pi/2
  if (pivot_angle < (lowest_angle - epsilon))
  {
    pivot_angle += M_PI;
    wheel_speed *= -1;
  }

  // pivot_angle approximately equals centre_angle - pi/2
  else if (pivot_angle <= (lowest_angle + epsilon))
  {
    if (not prefer_clockwise_rotation)
    {
      pivot_angle += M_PI;
      wheel_speed *= -1;
    }
  }

  // pivot_angle greater than centre_angle + pi/2
  else if (pivot_angle > (highest_angle + epsilon))
  {
    pivot_angle -= M_PI;
    wheel_speed *= -1;
  }

  // pivot_angle approximately equals centre_angle + pi/2
  else if (pivot_angle >= (highest_angle - epsilon))
  {
    if (prefer_clockwise_rotation)
    {
      pivot_angle -= M_PI;
      wheel_speed *= -1;
    }
  }
}

void wrap_pivot_angle(double& pivot_angle)
{
  pivot_angle = std::fmod(pivot_angle, 2 * M_PI);

  if (pivot_angle > M_PI)
  {
    pivot_angle -= 2 * M_PI;
  }
  else if (pivot_angle <= -M_PI)
  {
    pivot_angle += 2 * M_PI;
  }
}

// void clamp_pivot_angle(double& pivot_angle, const double& from, const double& to)
// {
//   if (from <= to)
//   {
//     pivot_angle = std::clamp(pivot_angle, from, to);
//   }
//   else
//   {
//
//   }
// }

double get_angle_between(const double& from, const double& to)
{
  double angle = to - from;

  if (angle > M_PI)
  {
    angle -= 2 * M_PI;
  }
  else if (angle < -M_PI)
  {
    angle += 2 * M_PI;
  }
  return angle;
}

}  // namespace holonomic_drive_controller

#endif  // HOLONOMIC_DRIVE_CONTROLLER__KINEMATICS_HPP_