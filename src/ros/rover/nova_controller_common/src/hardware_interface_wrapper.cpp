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
 * @authors Terry Tian
 */

#include <algorithm>
#include <functional>
#include <optional>
#include <cstddef>
#include <utility>

#include "nova_controller_common/hardware_interface_wrapper.hpp"

namespace nova_controller_common
{

HardwareInterfaceWrapper::HardwareInterfaceWrapper(
  rclcpp_lifecycle::LifecycleNode::SharedPtr node,
  std::vector<hardware_interface::LoanedStateInterface>& state_interfaces,
  std::vector<hardware_interface::LoanedCommandInterface>& command_interfaces)
  : node_(std::move(node))
  , state_interfaces_(state_interfaces)
  , command_interfaces_(command_interfaces)
{
}

bool HardwareInterfaceWrapper::set_value(double value, const size_t idx) const
{
  if (idx >= registered_handles_.size())
  {
    RCLCPP_ERROR(node_->get_logger(), "Index %zu out of bounds for registered handles", idx);
    return false;
  }
  if (!registered_handles_[idx])
  {
    RCLCPP_ERROR(node_->get_logger(), "Joint handle not registered for joint at index %zu", idx);
    return false;
  }

  registered_handles_[idx]->command.get().set_value(value);

  return true;
}

std::optional<double> HardwareInterfaceWrapper::get_optional(const size_t idx, bool cmd_if) const
{
  if (idx >= registered_handles_.size())
  {
    RCLCPP_ERROR(node_->get_logger(), "Index %zu out of bounds for registered handles", idx);
    return std::nullopt;
  }
  if (!registered_handles_[idx])
  {
    RCLCPP_ERROR(node_->get_logger(), "Joint handle not registered for joint at index %zu", idx);
    return std::nullopt;
  }

  double res;
  if (cmd_if)
  {
    const auto& command_handle = registered_handles_[idx]->command;
    res = command_handle.get().get_value();
  }
  else
  {
    const auto& state_handle = registered_handles_[idx]->state;
    if (!state_handle.has_value())
    {
      RCLCPP_ERROR(node_->get_logger(), "State handle not available for joint at index %zu", idx);
      return std::nullopt;
    }
    res = state_handle.value().get().get_value();
  }

  return std::make_optional(res);
}

bool HardwareInterfaceWrapper::configure_joint_handles(std::vector<Joint>& joints, bool open_loop)
{
  registered_handles_.resize(joints.size());

  for (const auto& [name, feedback_type, command_type, idx] : joints)
  {
    const auto cmd_iter = std::find_if(
      command_interfaces_.begin(), command_interfaces_.end(),
      [&name, &command_type](const auto& interface)
      {
        return interface.get_prefix_name() == name &&
               interface.get_interface_name() == command_type;
      });

    if (cmd_iter == command_interfaces_.end())
    {
      RCLCPP_ERROR(
        node_->get_logger(), "Unable to obtain joint command handle for %s", name.c_str());
      return false;
    }

    if (open_loop)
    {
      registered_handles_[idx] = WheelHandle{std::nullopt, std::ref(*cmd_iter)};
    }
    else
    {
      const auto state_iter = std::find_if(
        state_interfaces_.cbegin(), state_interfaces_.cend(),
        [&name, &feedback_type](const auto& interface)
        {
          return interface.get_prefix_name() == name &&
                 interface.get_interface_name() == feedback_type;
        });

      if (state_iter == state_interfaces_.cend())
      {
        RCLCPP_ERROR(
          node_->get_logger(), "Unable to obtain joint state handle for %s", name.c_str());
        return false;
      }

      registered_handles_[idx] =
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

}  // namespace nova_controller_common