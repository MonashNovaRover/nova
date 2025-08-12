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
#include "joint_space_control_mode_parameters.hpp"
#include <nova_interfaces/msg/arm_fk_velocity_targets.hpp>

namespace joint_space_control_mode
{
using namespace control_mode;

class JOINT_SPACE_CONTROL_MODE_PUBLIC JointSpaceControlMode : public ControlMode
{
public:
  JointSpaceControlMode();

  void publish_halt_message(const rclcpp::Time & now) const;

  return_type on_init() override;
  void on_configure_inputs(Inputs inputs) override;
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

  /// Helper struct to avoid duplicating code for the nearly identical logic for each joint
  struct JointHandle
  {
    std::string name;
    Axis::SharedPtr axis;
    double scale;
  };

  /// Tracks parameters
  std::shared_ptr<joint_space_control_mode::ParamListener> param_listener_{};
  joint_space_control_mode::Params params_;

  rclcpp::Publisher<nova_interfaces::msg::ArmFkVelocityTargets>::SharedPtr publisher_;

  /// Inputs for each joint in the message
  std::vector<JointHandle> joints_{};

  /// Names for each input
  std::vector<std::string> input_names_{};

  /// Input from 0 to 1 that directly scales the output speed.
  Axis::SharedPtr speed_;
};

}  // namespace joint_space_control_mode

#endif  // JOINT_SPACE_CONTROL_MODE__JOINT_SPACE_CONTROL_MODE_HPP_
