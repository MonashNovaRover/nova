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

#ifndef NOVA_CONTROLLER_COMMON__BLCMD_WRAPPER_HPP_
#define NOVA_CONTROLLER_COMMON__BLCMD_WRAPPER_HPP_

#include <algorithm>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

#include "controller_interface/controller_interface.hpp"
#include "hardware_interface/loaned_command_interface.hpp"
#include "hardware_interface/loaned_state_interface.hpp"
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

std::string to_string(const JointPosition& position)
{
  switch (position)
  {
    case JointPosition::FRONT_LEFT:
      return "front_left";
    case JointPosition::FRONT_RIGHT:
      return "front_right";
    case JointPosition::BACK_LEFT:
      return "back_left";
    case JointPosition::BACK_RIGHT:
      return "back_right";
    default:
      return "unknown";
  }
}

enum class JointType : uint8_t
{
  DRIVE,
  PIVOT
};

struct Joint
{
  std::string name;
  char* feedback_type;
  char* command_type;
  JointPosition position;
  JointType type;
}

struct WheelHandle
{
  std::optional<std::reference_wrapper<const hardware_interface::LoanedStateInterface>> state;
  std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
};

class BLCMDWrapper
{
public:
  BLCMDWrapper(
    rclcpp::Node::SharedPtr node, const std::vector<hardware_interface::LoanedStateInterface>& state_interfaces)
    const std::vector<hardware_interface::LoanedCommandInterface>& command_interfaces,
    : node_(node),
      state_interfaces_(state_interfaces),
      command_interfaces_(command_interfaces),
      registered_handles_(8, std::nullopt)
  {
  }

  template <typename T>
  bool set_value(const T& value, const JointPosition& joint_pos, const JointType& joint_type)
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
      return registered_handles_[index]->command.get().set_value(value * reverse_multiplier_);
    }
    return registered_handles_[index]->command.get().set_value(value);
  }

  template <typename T>
  std::optional<T> get_optional(const JointPosition& joint_pos, const JointType& joint_type) const
  {
    const size_t index = get_index(joint_pos, joint_type);
    if (!registered_handles_[index])
    {
      RCLCPP_ERROR(node_->get_logger(), "Joint handle not registered for joint at index %zu", index);
      return std::nullopt;
    }

    const auto& state_handle = registered_handles_[index]->state;
    if (!state_handle.has_value())
    {
      RCLCPP_ERROR(node_->get_logger(), "State handle not available for joint at index %zu", index);
      return std::nullopt;
    }

    const auto& res = state_handle.get().get_optional();

    // Invert pivot position value for BLCMDs
    if (joint_type == JointType::PIVOT)
    {
      return res.has_value() ? std::make_optional(res.value() * reverse_multiplier_) : std::nullopt;
    }
    return res.has_value() ? std::make_optional(res.value()) : std::nullopt;
  }

  bool configure_joint_handles(std::vector<JointConfig>& joint_configs, bool open_loop)
  {
    for (const auto& [joint_name, feedback_type, command_type, joint] : joint_configs)
    {
      const std::optional<std::reference_wrapper<const hardware_interface::LoanedStateInterface>> state_handle;
      const std::reference_wrapper<hardware_interface::LoanedCommandInterface> command_handle;

      if (open_loop)
      {
        state_handle = std::nullopt;
      }
      else
      {
        const auto state_iter = std::find_if(
          state_interfaces_.cbegin(), state_interfaces_.cend(),
          [&jc](const auto& interface)
          {
            return interface.get_prefix_name() == joint_name && interface.get_interface_name() == feedback_type;
          });

        if (state_iter == state_interfaces_.cend())
        {
          RCLCPP_ERROR(node_->get_logger(), "Unable to obtain joint state handle for %s", joint_name.c_str());
          return false;
        }
        state_handle = std::ref(*state_iter);
      }

      const auto cmd_iter = std::find_if(
        command_interfaces_.begin(), command_interfaces_.end(),
        [&jc](const auto& interface)
        {
          return interface.get_prefix_name() == joint_name && interface.get_interface_name() == command_type;
        });

      if (cmd_iter == command_interfaces_.end())
      {
        RCLCPP_ERROR(node_->get_logger(), "Unable to obtain joint command handle for %s", joint_name.c_str());
        return false;
      }
      command_handle = std::ref(*cmd_iter);

      registered_handles_[get_index(joint)] = WheelHandle{state_handle, command_handle};
    }

    return true;
  }

  void reset_handles()
  {
    for (auto& handle : registered_handles_)
    {
      handle.reset();
    }
  }

private:
  size_t get_index(const JointPosition& pos, JointType& type) const
  {
    // Position = 2 bits, type = 1 bit
    return (static_cast<size_t>(pos) << 1) | static_cast<size_t>(type);
  }

  rclcpp::Node::SharedPtr node_;
  std::vector<hardware_interface::LoanedCommandInterface>& command_interfaces_;
  std::vector<hardware_interface::LoanedStateInterface>& state_interfaces_;

  const int reverse_multiplier_ = -1;

  std::vector<std::optional<WheelHandle>> registered_handles_;
};

}  // namespace nova_controller_common

#endif  // NOVA_CONTROLLER_COMMON__BLCMD_WRAPPER_HPP_