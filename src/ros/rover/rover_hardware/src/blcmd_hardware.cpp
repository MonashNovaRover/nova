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
#include <callback.h>

#include "rover_hardware/blcmd_hardware.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

#include "jcan.h"

namespace rover_hardware
{
hardware_interface::CallbackReturn BLCMDHardware::on_init(
  const hardware_interface::HardwareInfo & info)
{
  if (hardware_interface::ActuatorInterface::on_init(info) != CallbackReturn::SUCCESS)
  {
    return CallbackReturn::ERROR;
  }

  if (info_.joints.size() != 1)
  {
    RCLCPP_FATAL_STREAM(
      rclcpp::get_logger(BLCMDHardwareLoggerName),
      "Hardware interface '" << info_.name << "got " << info_.joints.size() << " joints but expected 1");
    return CallbackReturn::ERROR;
  }

  auto canbus_search = info_.hardware_parameters.find("candevice");
  if (canbus_search == info.hardware_parameters.end()){
      RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No canbus provided");
      return CallbackReturn::ERROR;
  }
    std::stringstream joints_info;

  for(auto joint : info.joints) {
      joints_info << "--------------------"  << std::endl;
      joints_info << "Name: " << joint.name << ", Type: " << joint.type << std::endl;
      joints_info << "State Interfaces: ";
      for(auto interface : joint.state_interfaces){
          joints_info << interface.name << ", ";
      }
      joints_info << std::endl << "Command Interfaces: ";
      for(auto interface : joint.command_interfaces){
          joints_info << interface.name << ", ";
      }
      joints_info << std::endl <<  "--------------------"  << std::endl;
  }
  RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName), joints_info.str());
  can_device_ = canbus_search->second;
  RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
              "Using can device " << can_device_.c_str());

  bus_ = org::jcan::new_bus();

  for (const auto& interface : info_.joints[0].command_interfaces){
      if(interface.name == hardware_interface::HW_IF_POSITION){
          hw_position_command_ = std::numeric_limits<double>::quiet_NaN();
      } else if(interface.name == hardware_interface::HW_IF_VELOCITY){
            hw_velocity_command_ = std::numeric_limits<double>::quiet_NaN();
      } else if(interface.name == hardware_interface::HW_IF_EFFORT){
            hw_effort_command_ = std::numeric_limits<double>::quiet_NaN();
      }
  }

  for (const auto& interface : info_.joints[0].command_interfaces){
      if(interface.name == hardware_interface::HW_IF_POSITION){
          hw_position_state_ = std::numeric_limits<double>::quiet_NaN();
      } else if(interface.name == hardware_interface::HW_IF_VELOCITY){
            hw_velocity_state_ = std::numeric_limits<double>::quiet_NaN();
      } else if(interface.name == hardware_interface::HW_IF_EFFORT){
            hw_effort_state_ = std::numeric_limits<double>::quiet_NaN();
      }
  }

  control_mode_ = rover_hardware::ControlMode::Undefined;

  return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn BLCMDHardware::on_configure(
  const rclcpp_lifecycle::State & previous_state)
{
  // open the can bus
    try {
        bus_->open(this->can_device_.c_str());
        RCLCPP_INFO(rclcpp::get_logger(BLCMDHardwareLoggerName), "Opened canbus on device %s",
                    this->can_device_.c_str());
    } catch (std::exception &e) {
        RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "Failed to start canbus with error: %s",
                     e.what());
        return CallbackReturn::ERROR;
    }

    // if joint has a position interface, check for resolver

  return CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> BLCMDHardware::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  if (hw_position_state_.has_value()) state_interfaces.emplace_back(hardware_interface::StateInterface(
      info_.joints[0].name, hardware_interface::HW_IF_POSITION, &hw_position_state_.value()));
  if (hw_velocity_state_.has_value()) state_interfaces.emplace_back(hardware_interface::StateInterface(
      info_.joints[0].name, hardware_interface::HW_IF_VELOCITY, &hw_velocity_state_.value()));
  if (hw_effort_state_.has_value())state_interfaces.emplace_back(hardware_interface::StateInterface(
      info_.joints[0].name, hardware_interface::HW_IF_EFFORT, &hw_effort_state_.value()));

  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> BLCMDHardware::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;
    if (hw_position_command_.has_value()) command_interfaces.emplace_back(hardware_interface::CommandInterface(
        info_.joints[0].name, hardware_interface::HW_IF_POSITION, &hw_position_command_.value()));
    if (hw_velocity_command_.has_value()) command_interfaces.emplace_back(hardware_interface::CommandInterface(
        info_.joints[0].name, hardware_interface::HW_IF_VELOCITY, &hw_velocity_command_.value()));
    if (hw_effort_command_.has_value()) command_interfaces.emplace_back(hardware_interface::CommandInterface(
            info_.joints[0].name, hardware_interface::HW_IF_EFFORT, &hw_effort_command_.value()));

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
