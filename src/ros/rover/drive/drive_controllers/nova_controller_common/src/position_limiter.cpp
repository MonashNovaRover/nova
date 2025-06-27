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
 * @brief Positional counterpart to SpeedLimiter.
 * @authors Terry Tian
 */

#include <algorithm>
#include <stdexcept>

#include "nova_controller_common/position_limiter.hpp"

namespace nova_controller_common
{
PositionLimiter::PositionLimiter(
  bool has_velocity_limits, bool has_acceleration_limits, bool has_jerk_limits, double min_velocity,
  double max_velocity, double min_acceleration, double max_acceleration, double min_jerk,
  double max_jerk)
: has_velocity_limits_(has_velocity_limits),
  has_acceleration_limits_(has_acceleration_limits),
  has_jerk_limits_(has_jerk_limits),
  min_velocity_(min_velocity),
  max_velocity_(max_velocity),
  min_acceleration_(min_acceleration),
  max_acceleration_(max_acceleration),
  min_jerk_(min_jerk),
  max_jerk_(max_jerk)
{
  // Check if limits are valid, max must be specified, min defaults to -max if unspecified
  if (has_velocity_limits_)
  {
    if (std::isnan(max_velocity_))
    {
      throw std::runtime_error("Cannot apply velocity limits if max_velocity is not specified");
    }
    if (std::isnan(min_velocity_))
    {
      min_velocity_ = -max_velocity_;
    }
  }
  if (has_acceleration_limits_)
  {
    if (std::isnan(max_acceleration_))
    {
      throw std::runtime_error(
        "Cannot apply acceleration limits if max_acceleration is not specified");
    }
    if (std::isnan(min_acceleration_))
    {
      min_acceleration_ = -max_acceleration_;
    }
  }
  if (has_jerk_limits_)
  {
    if (std::isnan(max_jerk_))
    {
      throw std::runtime_error("Cannot apply jerk limits if max_jerk is not specified");
    }
    if (std::isnan(min_jerk_))
    {
      min_jerk_ = -max_jerk_;
    }
  }
}

double PositionLimiter::limit(double &p, const double &p0, const double &p1, const double &p2, const double &dt)
{
  const double dp = p - p0;
  
  limit_jerk(p, p0, p1, p2, dt);
  limit_acceleration(p, p0, p1, dt);
  limit_velocity(p, p0, dt);
  
  return dp != 0.0 ? (p - p0) / dp : 1.0;
}

double PositionLimiter::limit_velocity(double &p, const double &p0, const double &dt)
{
  const double dp = p - p0;

  if (has_velocity_limits_)
  {
    const double dp_min = min_velocity_ * dt;
    const double dp_max = max_velocity_ * dt;

    const double dp_lim = std::clamp(dp, dp_min, dp_max);

    p = p0 + dp_lim;
  }

  return dp != 0.0 ? (p - p0) / dp : 1.0;
}

double PositionLimiter::limit_acceleration(double &p, const double &p0, const double &p1, const double &dt)
{
  const double dp = p - p0;

  if (has_acceleration_limits_)
  {
    const double dp0 = p0 - p1;
    
    const double dt2 = dt * dt;
    const double dp_min = dp0 + min_acceleration_ * dt2;
    const double dp_max = dp0 + max_acceleration_ * dt2;

    const double dp_lim = std::clamp(dp, dp_min, dp_max);

    p = p0 + dp_lim;
  }

  return dp != 0.0 ? (p - p0) / dp : 1.0;
}

double PositionLimiter::limit_jerk(double &p, const double &p0, const double &p1, const double &p2, const double &dt)
{
  const double dp = p - p0;

  if (has_jerk_limits_)
  {
    const double dp0 = p0 - p1;
    const double dp1 = p1 - p2;

    const double d2p = dp - dp0;
    const double d2p0 = dp0 - dp1;

    const double d3p = d2p - d2p0;

    // Only limit jerk when accelerating or reverse_accelerating
    // Note: this prevents oscillating closed-loop behavior, see discussion
    // details in https://github.com/ros-controls/control_toolbox/issues/240.
    if (d3p * d2p > 0)
    {
      const double dt3 = dt * dt * dt;
      const double dp_min = 2*dp0 + dp1 + min_jerk_ * dt3;
      const double dp_max = 2*dp0 + dp1 + max_jerk_ * dt3;
  
      const double dp_lim = std::clamp(dp, dp_min, dp_max);
  
      p = p0 + dp_lim;
    }
  }

  return dp != 0.0 ? (p - p0) / dp : 1.0;
}

}  // namespace nova_controller_common
