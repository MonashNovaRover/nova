// Copyright (c) 2022, Stogl Robotics Consulting UG (haftungsbeschränkt) (template)
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

#ifndef ROVER_HARDWARE__BLCMD_HARDWARE_HPP_
#define ROVER_HARDWARE__BLCMD_HARDWARE_HPP_

#include <string>
#include <vector>
#include <optional>

#include "rclcpp/rclcpp.hpp"
#include "hardware_interface/actuator_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp/macros.hpp"
#include "rclcpp_lifecycle/state.hpp"

#include "jcan.h"

namespace rover_hardware
{
constexpr const char * BLCMDHardwareLoggerName = "BLCMDHardware";
struct PIConfig {
    int16_t kp;
    int16_t ki_ts;
    int16_t max_threshold;
};

struct BLCMDConfig {
    bool has_resolver;
    PIConfig current_config;
    PIConfig velocity_config;
    PIConfig position_config;
    int16_t min_interval;
};

struct JointValue {
    double position{0.0};
    double velocity{0.0};
    double current{0.0};
};

struct Joint {
    int16_t id;
    JointValue state;
    JointValue command;
};

enum class ControlMode {
    Undefined,
    Position,
    Velocity,
    Current,
};

class BLCMDHardware : public hardware_interface::ActuatorInterface
{
public:
  hardware_interface::CallbackReturn on_init(
    const hardware_interface::HardwareInfo & info) override;

  hardware_interface::CallbackReturn on_configure(
    const rclcpp_lifecycle::State & previous_state) override;

  std::vector<hardware_interface::StateInterface> export_state_interfaces() override;

  std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

  hardware_interface::CallbackReturn on_activate(
    const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::CallbackReturn on_deactivate(
    const rclcpp_lifecycle::State & previous_state) override;

  hardware_interface::return_type read(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

  hardware_interface::return_type write(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

private:
  std::optional<double> hw_effort_command_;
  std::optional<double> hw_velocity_command_;
  std::optional<double> hw_position_command_;
  std::optional<double> hw_effort_state_;
  std::optional<double> hw_velocity_state_;
  std::optional<double> hw_position_state_;

  ControlMode control_mode_;

  std::unique_ptr<org::jcan::Bus> bus_;
  std::basic_string<char> can_device_;

};

}  // namespace rover_hardware

#endif  // ROVER_HARDWARE__BLCMD_HARDWARE_HPP_
