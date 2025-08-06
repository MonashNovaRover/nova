// Copyright (c) 2020 PAL Robotics S.L.
//               2025 Monash Nova Rover
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
 * SpeedLimiter was originally written by Enrique Fernández from PAL Robotics.
 * 
 * @authors Terry Tian
 */

#ifndef NOVA_CONTROLLER_COMMON__POSITION_LIMITER_HPP_
#define NOVA_CONTROLLER_COMMON__POSITION_LIMITER_HPP_

#include <cmath>

namespace nova_controller_common
{

class PositionLimiter
{
public:
  /**
   * @brief Constructor
   * @param [in] has_velocity_limits     if true, applies velocity limits
   * @param [in] has_acceleration_limits if true, applies acceleration limits
   * @param [in] has_jerk_limits         if true, applies jerk limits
   * @param [in] min_velocity Minimum velocity [m/s], usually <= 0
   * @param [in] max_velocity Maximum velocity [m/s], usually >= 0
   * @param [in] min_acceleration Minimum acceleration [m/s^2], usually <= 0
   * @param [in] max_acceleration Maximum acceleration [m/s^2], usually >= 0
   * @param [in] min_jerk Minimum jerk [m/s^3], usually <= 0
   * @param [in] max_jerk Maximum jerk [m/s^3], usually >= 0
   */
  PositionLimiter(
    bool has_velocity_limits = false, bool has_acceleration_limits = false,
    bool has_jerk_limits = false, double min_velocity = NAN, double max_velocity = NAN,
    double min_acceleration = NAN, double max_acceleration = NAN, double min_jerk = NAN,
    double max_jerk = NAN)
    : has_velocity_limits_(has_velocity_limits)
    , has_acceleration_limits_(has_acceleration_limits)
    , has_jerk_limits_(has_jerk_limits)
    , min_velocity_(min_velocity)
    , max_velocity_(max_velocity)
    , min_acceleration_(min_acceleration)
    , max_acceleration_(max_acceleration)
    , min_jerk_(min_jerk)
    , max_jerk_(max_jerk)
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

  /**
   * @brief Limit the velocity, acceleration and jerk
   * @param [in, out] p  Position
   * @param [in]      p0 Previous position to p
   * @param [in]      p1 Previous position to p0
   * @param [in]      p2 Previous position to p1
   * @param [in]      dt Time step [s]
   * @return Limiting factor (1.0 if none)
   */
  double limit(double& p, double p0, double p1, double p2, double dt) const
  {
    const double dp = p - p0;

    limit_jerk(p, p0, p1, p2, dt);
    limit_acceleration(p, p0, p1, dt);
    limit_velocity(p, p0, dt);

    return dp != 0.0 ? (p - p0) / dp : 1.0;
  }

  /**
   * @brief Limit the velocity
   * @param [in, out] p  Position
   * @param [in]      p0 Previous position
   * @param [in]      dt Time step [s]
   * @return Limiting factor (1.0 if none)
   */
  double limit_velocity(double& p, double p0, double dt) const
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

  /**
   * @brief Limit the acceleration
   * @param [in, out] p  Position
   * @param [in]      p0 Previous position to p
   * @param [in]      p1 Previous position to p0
   * @param [in]      dt Time step [s]
   * @return Limiting factor (1.0 if none)
   */
  double limit_acceleration(double& p, double p0, double p1, double dt) const
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

  /**
   * @brief Limit the jerk
   * @param [in, out] p  Position
   * @param [in]      p0 Previous position to p
   * @param [in]      p1 Previous position to p0
   * @param [in]      p2 Previous position to p1
   * @param [in]      dt Time step [s]
   * @return Limiting factor (1.0 if none)
   * @see http://en.wikipedia.org/wiki/Jerk_%28physics%29#Motion_control
   */
  double limit_jerk(double& p, double p0, double p1, double p2, double dt) const
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
        const double dp_min = 2 * dp0 + dp1 + min_jerk_ * dt3;
        const double dp_max = 2 * dp0 + dp1 + max_jerk_ * dt3;

        const double dp_lim = std::clamp(dp, dp_min, dp_max);

        p = p0 + dp_lim;
      }
    }

    return dp != 0.0 ? (p - p0) / dp : 1.0;
  }

private:
  // Enable/Disable velocity/acceleration/jerk limits:
  bool has_velocity_limits_;
  bool has_acceleration_limits_;
  bool has_jerk_limits_;

  // Velocity limits:
  double min_velocity_;
  double max_velocity_;

  // Acceleration limits:
  double min_acceleration_;
  double max_acceleration_;

  // Jerk limits:
  double min_jerk_;
  double max_jerk_;
};

}  // namespace nova_controller_common

#endif  // NOVA_CONTROLLER_COMMON__POSITION_LIMITER_HPP_
