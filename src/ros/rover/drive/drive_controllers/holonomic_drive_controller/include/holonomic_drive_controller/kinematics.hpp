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

inline Eigen::Vector2d get_wheel_velocity(const Eigen::Vector2d& linear_velocity, const double angular_velocity, const Eigen::Vector2d& wheel_position)
{
  const Eigen::Vector2d angular_component {-angular_velocity * wheel_position.y(), angular_velocity * wheel_position.x()};
  return linear_velocity + angular_component;
}

inline double get_pivot_angle(const Eigen::Vector2d& wheel_velocity)
{
  if (wheel_velocity.norm() == 0)
  {
    return 0;
  }
  return std::atan2(wheel_velocity.y(), wheel_velocity.x());
}

inline void optimise_pivot_angle(double& pivot_angle, const double previous_angle, const std::vector<double>& angle_limits, const double epsilon, double& wheel_speed)
{
  int half_rotations = static_cast<int>(std::round((previous_angle - pivot_angle) / M_PI));

  pivot_angle += half_rotations * M_PI;

  if (pivot_angle < (angle_limits[0] - epsilon))
  {
    pivot_angle += M_PI;
    half_rotations += 1;
  }
  else if (pivot_angle < angle_limits[0])
  {
    pivot_angle = angle_limits[0];
  }
  else if (pivot_angle > (angle_limits[1] + epsilon))
  {
    pivot_angle -= M_PI;
    half_rotations -= 1;
  }
  else if (pivot_angle > angle_limits[1])
  {
    pivot_angle = angle_limits[1];
  }

  if (half_rotations % 2 != 0)
  {
    wheel_speed *= -1;
  }
}

inline void wrap_pivot_angle(double& pivot_angle)
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

inline double get_angle_between(const double& from, const double& to)
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