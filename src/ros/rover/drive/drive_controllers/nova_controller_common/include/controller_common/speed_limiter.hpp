// Copyright 2020 PAL Robotics S.L.
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

/*
 * Author: Enrique Fernández
 */

#ifndef NOVA_CONTROLLER_COMMON__SPEED_LIMITER_HPP_
#define NOVA_CONTROLLER_COMMON__SPEED_LIMITER_HPP_

#include <cmath>
#include <algorithm>
#include <stdexcept>

namespace nova_controller_common
{
  class SpeedLimiter
  {
  public:
    /**
     * \brief Constructor
     * \param [in] has_velocity_limits     if true, applies velocity limits
     * \param [in] has_acceleration_limits if true, applies acceleration limits
     * \param [in] has_jerk_limits         if true, applies jerk limits
     * \param [in] min_velocity Minimum velocity [m/s], usually <= 0
     * \param [in] max_velocity Maximum velocity [m/s], usually >= 0
     * \param [in] min_acceleration Minimum acceleration [m/s^2], usually <= 0
     * \param [in] max_acceleration Maximum acceleration [m/s^2], usually >= 0
     * \param [in] min_jerk Minimum jerk [m/s^3], usually <= 0
     * \param [in] max_jerk Maximum jerk [m/s^3], usually >= 0
     */
    SpeedLimiter(
      bool has_velocity_limits = false, bool has_acceleration_limits = false,
      bool has_jerk_limits = false, double min_velocity = NAN, double max_velocity = NAN,
      double min_acceleration = NAN, double max_acceleration = NAN, double min_jerk = NAN,
      double max_jerk = NAN)
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

    /**
     * \brief Limit the velocity and acceleration
     * \param [in, out] v  Velocity [m/s]
     * \param [in]      v0 Previous velocity to v  [m/s]
     * \param [in]      v1 Previous velocity to v0 [m/s]
     * \param [in]      dt Time step [s]
     * \return Limiting factor (1.0 if none)
     */
    double limit(double &v, double v0, double v1, double dt)
    {
      const double tmp = v;

      limit_jerk(v, v0, v1, dt);
      limit_acceleration(v, v0, dt);
      limit_velocity(v);

      return tmp != 0.0 ? v / tmp : 1.0;
    }

    /**
     * \brief Limit the velocity
     * \param [in, out] v Velocity [m/s]
     * \return Limiting factor (1.0 if none)
     */
    double limit_velocity(double &v)
    {
      const double tmp = v;

      if (has_velocity_limits_)
      {
        v = std::clamp(v, min_velocity_, max_velocity_);
      }

      return tmp != 0.0 ? v / tmp : 1.0;
    }

    /**
     * \brief Limit the acceleration
     * \param [in, out] v  Velocity [m/s]
     * \param [in]      v0 Previous velocity [m/s]
     * \param [in]      dt Time step [s]
     * \return Limiting factor (1.0 if none)
     */
    double limit_acceleration(double &v, double v0, double dt)
    {
      const double tmp = v;

      if (has_acceleration_limits_)
      {
        const double dv_min = min_acceleration_ * dt;
        const double dv_max = max_acceleration_ * dt;

        const double dv = std::clamp(v - v0, dv_min, dv_max);

        v = v0 + dv;
      }

      return tmp != 0.0 ? v / tmp : 1.0;
    }

    /**
     * \brief Limit the jerk
     * \param [in, out] v  Velocity [m/s]
     * \param [in]      v0 Previous velocity to v  [m/s]
     * \param [in]      v1 Previous velocity to v0 [m/s]
     * \param [in]      dt Time step [s]
     * \return Limiting factor (1.0 if none)
     * \see http://en.wikipedia.org/wiki/Jerk_%28physics%29#Motion_control
     */
    double limit_jerk(double &v, double v0, double v1, double dt)
    {
      const double tmp = v;
      
      if (has_jerk_limits_)
      {
        const double dv = v - v0;
        const double dv0 = v0 - v1;
        const double d2v = dv - dv0;
        
        // Only limit jerk when accelerating or reverse_accelerating
        // Note: this prevents oscillating closed-loop behavior, see discussion
        // details in https://github.com/ros-controls/control_toolbox/issues/240.
        if (d2v * dv > 0)
        {
          const double dt2 = dt * dt;
          const double d2v_min = min_jerk_ * dt2;
          const double d2v_max = max_jerk_ * dt2;
      
          const double d2v_lim = std::clamp(d2v, d2v_min, d2v_max);
      
          v = v0 + dv0 + d2v_lim;
        }
      }

      return tmp != 0.0 ? v / tmp : 1.0;
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

} // namespace nova_controller_common

#endif // NOVA_CONTROLLER_COMMON__SPEED_LIMITER_HPP_
