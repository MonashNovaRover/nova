// Copyright 2025 Bailey Chessum
// SPDX-License-Identifier: Apache-2.0
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
#ifndef JOINT_SPACE_CONTROL_MODE__JOINT_SPACE_CONTROL_MODE_HPP_
#define JOINT_SPACE_CONTROL_MODE__JOINT_SPACE_CONTROL_MODE_HPP_

#include <rclcpp/time.hpp>
#include "joint_space_control_mode/visibility_control.h"
#include "control_mode/control_mode.hpp"
#include "geometry_msgs/msg/vector3.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "joint_space_control_mode_parameters.hpp"

namespace joint_space_control_mode
{
using namespace control_mode;

class JOINT_SPACE_CONTROL_MODE_PUBLIC JointSpaceControlMode : public ControlMode
{
public:
  JointSpaceControlMode();

  void publish_halt_message(const rclcpp::Time & now) const;

  return_type on_init() override;
  void on_capture_inputs(Inputs inputs) override;
  return_type on_update(const rclcpp::Time & now, const rclcpp::Duration & period) override;

  CallbackReturn on_configure(const State & previous_state) override;
  CallbackReturn on_activate(const State & previous_state) override;
  CallbackReturn on_deactivate(const State & previous_state) override;
  CallbackReturn on_cleanup(const State & previous_state) override;
  CallbackReturn on_error(const State & previous_state) override;
  CallbackReturn on_shutdown(const State & previous_state) override;

protected:
  ~JointSpaceControlMode() override;

private:
  /// We use infinity as the default limit, as it will effectively not impose a limit while avoiding branching
  static constexpr double infinity = std::numeric_limits<double>::infinity();
  /// Helper type to hold 3 Axis::SharedPtrs to make a up a vector
  using AxisVector3 = std::array<Axis::SharedPtr, 3>;
  /// Helper type to hold 3 Axis::SharedPtrs to make a up a vector. You could also use Eigen for more complex use cases.
  using NumberVector3 = std::array<double, 3>;

  /// Helper struct to avoid duplicating code for the nearly identical logic for linear and angular components of twist.
  struct VectorHandle
  {
    /// Our actual inputs for the vector3
    AxisVector3 axes;
    /// A scale to multiply axis values by when populating vector3 messages
    NumberVector3 scale = {1.0, 1.0, 1.0};
    /// limits to apply to the vector3 message
    std::optional<NumberVector3> limits = std::nullopt;

    /// Switches limiting logic from simple component-wise limiting to more complex normalization based limits.
    bool normalized_limits = true;
    /// When true, limits will be applied to the axis inputs relative to the 'speed' input.
    bool scale_limits_with_speed = true;

    /**
     * \brief Given the parameter values, converts them to usable limits for apply_to().
     *
     * Any negative parameter values will translate to an infinity -- which is effectively no limit.
     * If all limits end up being infinity, limits will become std::nullopt.
     *
     * \param values[in]    The x,y,z limit parameters all put into an array<double, 3>
     * \param all[in]       The 'all' default limit parameter to use for any unspecified x,y,z in values
     */
    void set_limits(NumberVector3 values, double all);

    /**
     * \brief Calculates the value for a Vector3 message based on the input axis values, and given speed_coefficient.
     *
     * This method applies limits when limit.has_value().
     *
     * \param msg[out]  The message to put calculated values in
     * \param speed_coefficient[in]     The value to multiply all speeds by
     */
    void apply_to(geometry_msgs::msg::Vector3 & msg, double speed_coefficient);
  };

  /// Helper function to get the euclidean length of a vector, used for normalized limits.
  static double norm(double x, double y, double z);

  /// Tracks parameters
  std::shared_ptr<joint_space_control_mode::ParamListener> param_listener_{};
  joint_space_control_mode::Params params_;

  rclcpp::Publisher<geometry_msgs::msg::TwistStamped>::SharedPtr stamped_publisher_;
  rclcpp::Publisher<geometry_msgs::msg::Twist>::SharedPtr publisher_;

  /// Inputs for all the linear twist components
  VectorHandle linear_;
  /// Inputs for all the angular twist components
  VectorHandle angular_;

  /// Input from 0 to 1 that directly scales the output speed.
  Axis::SharedPtr speed_;
};

}  // namespace joint_space_control_mode

#endif  // JOINT_SPACE_CONTROL_MODE__JOINT_SPACE_CONTROL_MODE_HPP_
