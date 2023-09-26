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

#include <limits>
#include <vector>

#include "rover_hardware/blcmd_hardware.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

namespace rover_hardware
{
hardware_interface::CallbackReturn BLCMDHardware::on_init(
  const hardware_interface::HardwareInfo & info)
{
  if (hardware_interface::ActuatorInterface::on_init(info) != CallbackReturn::SUCCESS)
  {
    return CallbackReturn::ERROR;
  }

  auto canbus_search = info.hardware_parameters.find("candevice");
  if (canbus_search == info.hardware_parameters.end()){
      RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No canbus provided");
      return CallbackReturn::ERROR;
  }
  for(auto joint : info.joints) {
      std::cout << "--------------------"  << std::endl;
      std::cout << "Name: " << joint.name << ", Type: " << joint.type << std::endl;
      std::cout << "State Interfaces: ";
      for(auto interface : joint.state_interfaces){
          std::cout << interface.name << ", ";
      }
      std::cout << std::endl << "Command Interfaces: ";
      for(auto interface : joint.command_interfaces){
          std::cout << interface.name << ", ";
      }
      std::cout << std::endl <<  "--------------------"  << std::endl;
  }
  this->candevice = canbus_search->second;
  RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
              "using can device " << this->candevice.c_str());

  this->bus_ = org::jcan::new_bus();

  // TODO(anyone): read parameters and initialize the hardware
  hw_velocity_states_.resize(info_.joints.size(), std::numeric_limits<double>::quiet_NaN());
  hw_commands_.resize(info_.joints.size(), std::numeric_limits<double>::quiet_NaN());

  return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn BLCMDHardware::on_configure(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  // TODO(anyone): prepare the robot to be ready for read calls and write calls of some interfaces

  return CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> BLCMDHardware::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  for (size_t i = 0; i < info_.joints.size(); ++i)
  {
    state_interfaces.emplace_back(hardware_interface::StateInterface(
      // TODO(anyone): insert correct interfaces
      info_.joints[i].name, hardware_interface::HW_IF_POSITION, &hw_velocity_states_[i]));
  }

  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> BLCMDHardware::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;
  for (size_t i = 0; i < info_.joints.size(); ++i)
  {
    command_interfaces.emplace_back(hardware_interface::CommandInterface(
      // TODO(anyone): insert correct interfaces
      info_.joints[i].name, hardware_interface::HW_IF_POSITION, &hw_commands_[i]));
  }

  return command_interfaces;
}

hardware_interface::CallbackReturn BLCMDHardware::on_activate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  // TODO(anyone): prepare the robot to receive commands

  return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn BLCMDHardware::on_deactivate(
  const rclcpp_lifecycle::State & /*previous_state*/)
{
  // TODO(anyone): prepare the robot to stop receiving commands

  return CallbackReturn::SUCCESS;
}

hardware_interface::return_type BLCMDHardware::read(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  // TODO(anyone): read robot states

  return hardware_interface::return_type::OK;
}

hardware_interface::return_type BLCMDHardware::write(
  const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
  // TODO(anyone): write robot's commands'

  return hardware_interface::return_type::OK;
}

}  // namespace rover_hardware

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  rover_hardware::BLCMDHardware, hardware_interface::ActuatorInterface)
