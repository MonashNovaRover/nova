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
 * @authors Terry Tian
 */

#ifndef PIVOT_DRIVE_CONTROLLER__KINEMATICS_HPP_
#define PIVOT_DRIVE_CONTROLLER__KINEMATICS_HPP_

#include <cmath>

#include "nova_controller_common/position_limiter.hpp"

namespace pivot_drive_controller
{

constexpr double get_angular_from_radius_and_speed(
  double radius, double speed, bool turning_left, double zero_radius)
{
  int dir = turning_left ? 1 : -1;
  if (radius == INFINITY)
  {
    return 0.0;  // straight line, no angular velocity
  }
  if (radius == 0)
  {
    return (speed / zero_radius) * dir;  // turning on the spot
  }
  return speed / radius;
}

constexpr double get_radius_from_velocities(double linear_velocity, double angular_velocity)
{
  if (angular_velocity == 0)
  {
    return INFINITY;  // straight line, no radius
  }
  if (linear_velocity == 0)
  {
    return 0.0;  // turning on the spot
  }
  return linear_velocity / angular_velocity;
}

constexpr double get_pivot_angle_from_radius(
  double radius, bool left_pivot, bool turning_left, double half_steering_track,
  double half_wheel_base)
{
  if (radius == INFINITY)
  {
    return 0.0;  // straight line, no pivot angle
  }
  radius -= (left_pivot ? 1 : -1) * half_steering_track;
  return ((turning_left ? 1 : -1) * M_PI_2) - std::atan(radius / half_wheel_base);
}

constexpr double get_radius_from_pivot_angle(
  double angle, bool left_pivot, bool turning_left, double half_steering_track,
  double half_wheel_base, double epsilon = 1e-6)
{
  // Calculate the radius of the turn based on the angle of the pivot
  if (std::abs(angle) < epsilon)
  {
    return INFINITY;  // straight line
  }
  return (std::tan(((turning_left ? 1 : -1) * M_PI_2) - angle) * half_wheel_base) +
         ((left_pivot ? 1 : -1) * half_steering_track);
}

constexpr double get_speed_ratio(
  double radius, bool left_pivot, double half_steering_track, double half_wheel_base)
{
  if (radius == INFINITY || radius == 0)
  {
    // straight line or turning on the spot, left and right wheels should be the same speed
    return 1.0;
  }
  double wheel_turn_radius = radius - ((left_pivot ? 1 : -1) * half_steering_track);
  return std::abs(std::hypot(wheel_turn_radius, half_wheel_base) / radius);
}

inline void limit_radius_by_pivot(
  double& turning_radius, bool turning_left, double half_steering_track, double half_wheel_base,
  const nova_controller_common::PositionLimiter& limiter_pivot, double p0, double p1, double p2,
  double dt)
{
  // Limit the pivot that needs to turn more, i.e. the one on the same side as the turning direction
  // If the requested radius is a straight line, we limit based on the previous pivot position.
  bool left_pivot = turning_radius == INFINITY ? p0 > 0 : turning_left;
  double angle = get_pivot_angle_from_radius(
    turning_radius, left_pivot, turning_left, half_steering_track, half_wheel_base);

  limiter_pivot.limit(angle, p0, p1, p2, dt);

  // Recalculate the turning radius based on the limited pivot angle
  turning_radius = get_radius_from_pivot_angle(
    angle, left_pivot, turning_left, half_steering_track, half_wheel_base);
  if (std::abs(turning_radius) < 1e-6)
  {
    turning_radius = 0.0;
  }
}

}  // namespace pivot_drive_controller

#endif  // PIVOT_DRIVE_CONTROLLER__KINEMATICS_HPP_