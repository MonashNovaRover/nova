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

enum class JointPosition : uint8_t
{
  FRONT_LEFT,
  FRONT_RIGHT,
  BACK_LEFT,
  BACK_RIGHT
};

JointPosition to_joint_position(const std::string& joint_name)
{
  if (joint_name == "front_left")
  {
    return JointPosition::FRONT_LEFT;
  }
  if (joint_name == "front_right")
  {
    return JointPosition::FRONT_RIGHT;
  }
  if (joint_name == "back_left")
  {
    return JointPosition::BACK_LEFT;
  }
  if (joint_name == "back_right")
  {
    return JointPosition::BACK_RIGHT;
  }
  throw std::invalid_argument("Invalid joint name: " + joint_name);
}

enum class JointType : uint8_t
{
  DRIVE,
  PIVOT
};

struct Joint
{
  const std::string name;
  const char* feedback_type;
  const char* command_type;
  const JointPosition position;
  const JointType type;

  Joint(
    const std::string& joint_name, const char* feedback, const char* command,
    const JointPosition joint_pos, JointType joint_type)
    : name(joint_name)
    , feedback_type(feedback)
    , command_type(command)
    , position(joint_pos)
    , type(joint_type)
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

  bool set_value(double value, const JointPosition joint_pos, const JointType joint_type) const;

  std::optional<double> get_optional(
    const JointPosition joint_pos, const JointType joint_type, bool cmd_if = false) const;

  bool configure_joint_handles(std::vector<Joint>& joints, bool open_loop);

  void reset_handles();

private:
  std::size_t get_index(const JointPosition& pos, const JointType& type) const;

  rclcpp_lifecycle::LifecycleNode::SharedPtr node_;
  std::vector<hardware_interface::LoanedCommandInterface>& command_interfaces_;
  std::vector<hardware_interface::LoanedStateInterface>& state_interfaces_;
  double offset_angle_;

  const int REVERSE_MULTIPLIER_ = -1;  // our BLCMD pivot positions are inverted

  std::vector<std::optional<WheelHandle>> registered_handles_;
};

}  // namespace nova_controller_common

#endif  // NOVA_CONTROLLER_COMMON__HARDWARE_INTERFACE_WRAPPER_HPP_