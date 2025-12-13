// Copyright 2020 PAL Robotics S.L.
//           2025 Monash Nova Rover
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
 * @brief Limits 2 dimensional velocities
 * SpeedLimiter was originally written by Enrique Fernández from PAL Robotics.
 *
 * @authors Jonathan Jia
 * @date Created 2025
 */

#ifndef NOVA_CONTROLLER_COMMON__VELOCITY_LIMITER_HPP_
#define NOVA_CONTROLLER_COMMON__VELOCITY_LIMITER_HPP_

#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <Eigen/Dense>

namespace nova_controller_common
{

class VelocityLimiter
{
public:
  /**
   * \brief Constructor
   * \param [in] has_velocity_limits     if true, applies velocity limits
   * \param [in] has_acceleration_limits if true, applies acceleration limits
   * \param [in] has_jerk_limits         if true, applies jerk limits
   * \param [in] max_velocity Maximum velocity [m/s]
   * \param [in] max_acceleration Maximum acceleration [m/s^2]
   * \param [in] max_jerk Maximum jerk [m/s^3]
   */
  VelocityLimiter(
    const bool has_velocity_limits = false, const bool has_acceleration_limits = false,
    const bool has_jerk_limits = false, const double max_velocity = NAN,
    const double max_acceleration = NAN, const double max_jerk = NAN)
    : has_velocity_limits_(has_velocity_limits)
    , has_acceleration_limits_(has_acceleration_limits)
    , has_jerk_limits_(has_jerk_limits)
    , max_velocity_(max_velocity)
    , max_acceleration_(max_acceleration)
    , max_jerk_(max_jerk)
  {
    // Check if limits are valid, max must be specified
    if (has_velocity_limits_)
    {
      if (std::isnan(max_velocity_))
      {
        throw std::runtime_error("Cannot apply velocity limits if max_velocity is not specified");
      }
    }
    if (has_acceleration_limits_)
    {
      if (std::isnan(max_acceleration_))
      {
        throw std::runtime_error(
          "Cannot apply acceleration limits if max_acceleration is not specified");
      }
    }
    if (has_jerk_limits_)
    {
      if (std::isnan(max_jerk_))
      {
        throw std::runtime_error("Cannot apply jerk limits if max_jerk is not specified");
      }
    }
  }

  /**
   * \brief Limit the velocity and acceleration
   * \param [in, out] v  Velocity [m/s]
   * \param [in]      v0 Previous velocity to v  [m/s]
   * \param [in]      v1 Previous velocity to v0 [m/s]
   * \param [in]      dt Time step [s]
   */
  void limit(Eigen::Vector2d& v, const Eigen::Vector2d& v0, const Eigen::Vector2d& v1, const double dt) const
  {
    limit_jerk(v, v0, v1, dt);
    limit_acceleration(v, v0, dt);
    limit_velocity(v);
  }

  /**
   * \brief Limit the velocity
   * \param [in, out] v Velocity [m/s]
   */
  void limit_velocity(Eigen::Vector2d& v) const
  {
    if (has_velocity_limits_ && v.norm() > max_velocity_)
    {
      v.normalize();
      v *= max_velocity_;
    }
  }

  /**
   * \brief Limit the acceleration
   * \param [in, out] v  Velocity [m/s]
   * \param [in]      v0 Previous velocity [m/s]
   * \param [in]      dt Time step [s]
   */
  void limit_acceleration(Eigen::Vector2d& v, const Eigen::Vector2d& v0, const double dt) const
  {
    if (has_acceleration_limits_)
    {
      const double dv_max = max_acceleration_ * dt;
      const Eigen::Vector2d dv = v - v0;
      if (dv.norm() > dv_max)
      {
        const Eigen::Vector2d dv_lim = dv.normalized() * dv_max;
        v = v0 + dv_lim;
      }
    }
  }

  /**
   * \brief Limit the jerk
   * \param [in, out] v  Velocity [m/s]
   * \param [in]      v0 Previous velocity to v  [m/s]
   * \param [in]      v1 Previous velocity to v0 [m/s]
   * \param [in]      dt Time step [s]
   * \see http://en.wikipedia.org/wiki/Jerk_%28physics%29#Motion_control
   */
  void limit_jerk(Eigen::Vector2d& v, const Eigen::Vector2d& v0, const Eigen::Vector2d& v1, const double dt) const
  {
    if (has_jerk_limits_)
    {
      const Eigen::Vector2d dv = v - v0;
      const Eigen::Vector2d dv0 = v0 - v1;
      const Eigen::Vector2d d2v = dv - dv0;

      // Only limit jerk when accelerating or reverse_accelerating
      // Note: this prevents oscillating closed-loop behavior, see discussion
      // details in https://github.com/ros-controls/control_toolbox/issues/240.
      if (d2v.x() * dv.x() > 0 && d2v.y() * dv.y() > 0)
      {
        const double d2v_max = max_jerk_ * dt * dt;

        if (d2v.norm() > d2v_max)
        {
          const Eigen::Vector2d d2v_lim = d2v.normalized() * d2v_max;
          v = v0 + dv0 + d2v_lim;
        }
      }
    }
  }

private:
  // Enable/Disable velocity/acceleration/jerk limits:
  bool has_velocity_limits_;
  bool has_acceleration_limits_;
  bool has_jerk_limits_;

  // Limits:
  double max_velocity_;
  double max_acceleration_;
  double max_jerk_;
};

}  // namespace nova_controller_common

#endif  // NOVA_CONTROLLER_COMMON__VELOCITY_LIMITER_HPP_
