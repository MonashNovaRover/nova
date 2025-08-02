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

#ifndef NOVA_CONTROLLER_COMMON__HARDWARE_INTERFACE_WRAPPER_HPP_
#define NOVA_CONTROLLER_COMMON__HARDWARE_INTERFACE_WRAPPER_HPP_

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>
#include <cstddef>

#include "hardware_interface/loaned_command_interface.hpp"
#include "hardware_interface/loaned_state_interface.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp/rclcpp.hpp"

namespace nova_controller_common
{

enum class JointSide
{
  LEFT,
  RIGHT
};

enum class JointType
{
  DRIVE,
  PIVOT
};

constexpr size_t encoded_pos(const size_t pos, const JointSide side, const JointType type)
{
  return pos << 2 | (static_cast<size_t>(side) << 1) | static_cast<size_t>(type);
}

struct Joint
{
  const std::string name;
  const char* feedback_type;
  const char* command_type;
  const size_t pos;
  const JointSide side;
  const JointType type;

  Joint(
    const std::string& joint_name, const char* feedback, const char* command, const size_t pos,
    const JointSide side, const JointType type)
    : name(joint_name)
    , feedback_type(feedback)
    , command_type(command)
    , pos(pos)
    , side(side)
    , type(type)
  {
  }
};

struct WheelHandle
{
  std::optional<std::reference_wrapper<const hardware_interface::LoanedStateInterface>> state;
  std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
};

class HardwareInterfaceWrapper
{
public:
  HardwareInterfaceWrapper(
    rclcpp_lifecycle::LifecycleNode::SharedPtr node, const double offset_angle,
    std::vector<hardware_interface::LoanedStateInterface>& state_interfaces,
    std::vector<hardware_interface::LoanedCommandInterface>& command_interfaces);

  bool set_value(double value, const size_t pos, const JointSide side, const JointType type) const;

  std::optional<double> get_optional(
    const size_t pos, const JointSide side, const JointType type, bool cmd_if = false) const;

  bool configure_joint_handles(std::vector<Joint>& joints, bool open_loop);

  void reset_handles();

private:
  rclcpp_lifecycle::LifecycleNode::SharedPtr node_;
  std::vector<hardware_interface::LoanedCommandInterface>& command_interfaces_;
  std::vector<hardware_interface::LoanedStateInterface>& state_interfaces_;
  double offset_angle_;

  const int REVERSE_MULTIPLIER_ = -1;  // our BLCMD pivot positions are inverted

  std::vector<std::optional<WheelHandle>> registered_handles_;
};

}  // namespace nova_controller_common

#endif  // NOVA_CONTROLLER_COMMON__HARDWARE_INTERFACE_WRAPPER_HPP_