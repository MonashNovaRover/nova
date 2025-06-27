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
        double max_jerk = NAN);

    /**
     * @brief Limit the velocity, acceleration and jerk
     * @param [in, out] p  Position
     * @param [in]      p0 Previous position to p
     * @param [in]      p1 Previous position to p0
     * @param [in]      p2 Previous position to p1
     * @param [in]      dt Time step [s]
     * @return Limiting factor (1.0 if none)
     */
    double limit(double &p, const double &p0, const double &p1, const double &p2, const double &dt);

    /**
     * @brief Limit the velocity
     * @param [in, out] p  Position
     * @param [in]      p0 Previous position
     * @param [in]      dt Time step [s]
     * @return Limiting factor (1.0 if none)
     */
    double limit_velocity(double &p, const double &p0, const double &dt);

    /**
     * @brief Limit the acceleration
     * @param [in, out] p  Position
     * @param [in]      p0 Previous position to p
     * @param [in]      p1 Previous position to p0
     * @param [in]      dt Time step [s]
     * @return Limiting factor (1.0 if none)
     */
    double limit_acceleration(double &p, const double &p0, const double &p1, const double &dt);

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
    double limit_jerk(double &p, const double &p0, const double &p1, const double &p2, const double &dt);

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

#endif // NOVA_CONTROLLER_COMMON__POSITION_LIMITER_HPP_
