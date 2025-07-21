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

#include <algorithm>
#include <functional>
#include <optional>
#include <cstddef>

#include "nova_controller_common/hardware_interface_wrapper.hpp"

namespace nova_controller_common
{

HardwareInterfaceWrapper::HardwareInterfaceWrapper(
  const rclcpp_lifecycle::LifecycleNode::SharedPtr node, const double offset_angle,
  std::vector<hardware_interface::LoanedStateInterface>& state_interfaces,
  std::vector<hardware_interface::LoanedCommandInterface>& command_interfaces)
  : node_(node)
  , offset_angle_(offset_angle)
  , state_interfaces_(state_interfaces)
  , command_interfaces_(command_interfaces)
  , registered_handles_(8, std::nullopt)
  , REVERSE_MULTIPLIER_(-1)
{
}

bool HardwareInterfaceWrapper::set_value(
  double value, const JointPosition joint_pos, const JointType joint_type) const
{
  const size_t index = get_index(joint_pos, joint_type);
  if (!registered_handles_[index])
  {
    RCLCPP_ERROR(node_->get_logger(), "Joint handle not registered for joint at index %zu", index);
    return false;
  }

  // Invert pivot position value for BLCMDs
  if (joint_type == JointType::PIVOT)
  {
    if (joint_pos == JointPosition::FRONT_LEFT || joint_pos == JointPosition::BACK_RIGHT)
    {
      value -= offset_angle_;
    }
    else
    {
      value += offset_angle_;
    }
    value *= REVERSE_MULTIPLIER_;
  }
  registered_handles_[index]->command.get().set_value(value);

  return true;
}

std::optional<double> HardwareInterfaceWrapper::get_optional(
  const JointPosition joint_pos, const JointType joint_type, bool cmd_if) const
{
  const size_t index = get_index(joint_pos, joint_type);
  if (!registered_handles_[index])
  {
    RCLCPP_ERROR(node_->get_logger(), "Joint handle not registered for joint at index %zu", index);
    return std::nullopt;
  }

  double res;
  if (cmd_if)
  {
    const auto& command_handle = registered_handles_[index]->command;
    res = command_handle.get().get_value();
  }
  else
  {
    const auto& state_handle = registered_handles_[index]->state;
    if (!state_handle.has_value())
    {
      RCLCPP_ERROR(node_->get_logger(), "State handle not available for joint at index %zu", index);
      return std::nullopt;
    }
    res = state_handle.value().get().get_value();
  }

  // Invert pivot position value for BLCMDs
  if (joint_type == JointType::PIVOT)
  {
    double offset_angle = offset_angle_;
    if (joint_pos == JointPosition::FRONT_LEFT || joint_pos == JointPosition::BACK_RIGHT)
    {
      res -= offset_angle_;
    }
    else
    {
      res += offset_angle_;
    }
    res *= REVERSE_MULTIPLIER_;
  }

  return std::make_optional(res);
}

bool HardwareInterfaceWrapper::configure_joint_handles(std::vector<Joint>& joints, bool open_loop)
{
  for (const auto& [joint_name, feedback_type, command_type, joint_pos, joint_type] : joints)
  {
    const auto cmd_iter = std::find_if(
      command_interfaces_.begin(), command_interfaces_.end(),
      [&joint_name, &command_type](const auto& interface)
      {
        return interface.get_prefix_name() == joint_name &&
               interface.get_interface_name() == command_type;
      });

    if (cmd_iter == command_interfaces_.end())
    {
      RCLCPP_ERROR(
        node_->get_logger(), "Unable to obtain joint command handle for %s", joint_name.c_str());
      return false;
    }

    if (open_loop)
    {
      registered_handles_[get_index(joint_pos, joint_type)] =
        WheelHandle{std::nullopt, std::ref(*cmd_iter)};
    }
    else
    {
      const auto state_iter = std::find_if(
        state_interfaces_.cbegin(), state_interfaces_.cend(),
        [&joint_name, &feedback_type](const auto& interface)
        {
          return interface.get_prefix_name() == joint_name &&
                 interface.get_interface_name() == feedback_type;
        });

      if (state_iter == state_interfaces_.cend())
      {
        RCLCPP_ERROR(
          node_->get_logger(), "Unable to obtain joint state handle for %s", joint_name.c_str());
        return false;
      }

      registered_handles_[get_index(joint_pos, joint_type)] =
        WheelHandle{std::make_optional(std::ref(*state_iter)), std::ref(*cmd_iter)};
    }
  }

  return true;
}

void HardwareInterfaceWrapper::reset_handles()
{
  for (auto& handle : registered_handles_)
  {
    handle.reset();
  }
}

std::size_t HardwareInterfaceWrapper::get_index(const JointPosition& pos, const JointType& type) const
{
  // Position = 2 bits, type = 1 bit
  return (static_cast<size_t>(pos) << 1) | static_cast<size_t>(type);
}

}  // namespace nova_controller_common