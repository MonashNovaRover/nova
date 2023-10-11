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
#include <unistd.h>

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
    if (canbus_search == info_.hardware_parameters.end()){
        RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No canbus provided");
        return CallbackReturn::ERROR;
    }

    can_device_ = canbus_search->second;
    RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                "Using can device " << can_device_.c_str());

    auto canid_search = info_.hardware_parameters.find("canid");
    if (canid_search == info_.hardware_parameters.end()){
        RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No canid provided");
        return CallbackReturn::ERROR;
    }

    can_id_ = std::stoi(canid_search->second);

    bus_ = org::jcan::new_bus();

    for (const auto& interface : info_.joints[0].command_interfaces){
        if(!set_control_interface(interface, true)){
            return CallbackReturn::ERROR;
        }
    }

    for (const auto& interface : info_.joints[0].state_interfaces){
        if(!set_control_interface(interface, false)){
            return CallbackReturn::ERROR;
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
        bus_->open(can_device_.c_str());
        RCLCPP_INFO(rclcpp::get_logger(BLCMDHardwareLoggerName), "Opened canbus on device %s",
                    can_device_.c_str());
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
  if (hw_position_.state.has_value()) {
      state_interfaces.emplace_back(
              info_.joints[0].name, hardware_interface::HW_IF_POSITION, &hw_position_.state.value());
  }
  if (hw_velocity_.state.has_value()) {
      state_interfaces.emplace_back(
              info_.joints[0].name, hardware_interface::HW_IF_VELOCITY, &hw_velocity_.state.value());
  }
  if (hw_effort_.state.has_value()) {
      state_interfaces.emplace_back(
              info_.joints[0].name, hardware_interface::HW_IF_EFFORT, &hw_effort_.state.value());
  }

  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> BLCMDHardware::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;
    if (hw_position_.command.has_value()) {
        command_interfaces.emplace_back(
                info_.joints[0].name, hardware_interface::HW_IF_POSITION, &hw_position_.command.value());
    }
    if (hw_velocity_.command.has_value()) {
        command_interfaces.emplace_back(
                info_.joints[0].name, hardware_interface::HW_IF_VELOCITY, &hw_velocity_.command.value());
    }
    if (hw_effort_.command.has_value()) {
        command_interfaces.emplace_back(
                info_.joints[0].name, hardware_interface::HW_IF_EFFORT, &hw_effort_.command.value());
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

// TODO: better error handling
bool BLCMDHardware::set_control_interface(
        const hardware_interface::InterfaceInfo &interface_info, bool command) {
        RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                           "Setting interface " << interface_info.name.c_str() << " with min " << interface_info.min <<
                           " and max " << interface_info.max << " as " << (command ? "command" : "state") << " interface");
    double min = std::stod(interface_info.min);
    double max = std::stod(interface_info.max);
    if(interface_info.name == hardware_interface::HW_IF_POSITION){
        //TODO: deal with case with state interface and no command interface
        if (command){
            hw_position_.min = min;
            hw_position_.max = max;
            hw_position_.resolver_reduction = std::stod(info_.joints[0].parameters.at("resolver_reduction"));
            hw_position_.command = std::numeric_limits<double>::quiet_NaN();
        } else {
            hw_position_.state = std::numeric_limits<double>::quiet_NaN();
        }
    } else if(interface_info.name == hardware_interface::HW_IF_VELOCITY){
        if(command) {
            hw_velocity_.min = min;
            hw_velocity_.max = max;
            hw_velocity_.command = std::numeric_limits<double>::quiet_NaN();
        } else {
            hw_velocity_.state = std::numeric_limits<double>::quiet_NaN();
        }
    } else if(interface_info.name == hardware_interface::HW_IF_EFFORT){
        if(command){
            hw_effort_.min = min;
            hw_effort_.max = max;
            hw_effort_.command = std::numeric_limits<double>::quiet_NaN();
        } else {
            hw_effort_.state = std::numeric_limits<double>::quiet_NaN();
        }
    } else {
        RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "Unexpected interface %s",
                     interface_info.name.c_str());
        return false;
    }
    return true;
}

    void BLCMDHardware::can_setup() {
        std::vector<uint32_t> ids = {make_can_id(BLCMDReceiveCommand::CONFIG_DATA)};
        if (hw_velocity_.state.has_value() || hw_effort_.state.has_value())
            ids.push_back(make_can_id(TelemetryPacket::PACKET_1));
        if (hw_position_.state.has_value())
            ids.push_back(make_can_id(TelemetryPacket::PACKET_3));
        bus_->set_id_filter(ids);
        if (hw_velocity_.state.has_value() || hw_effort_.state.has_value())
            bus_->add_callback_to(make_can_id(TelemetryPacket::PACKET_1), this, &BLCMDHardware::packet_1_callback);
        if (hw_position_.state.has_value())
            bus_->add_callback_to(make_can_id(TelemetryPacket::PACKET_3), this, &BLCMDHardware::packet3_callback);
   }

    uint32_t BLCMDHardware::make_can_id(BLCMDSendCommand command)
    {
        return static_cast<uint32_t>(CanIdPrefix::SEND) << 8 | can_id_ << 4 |
               static_cast<uint32_t>(command);
    }

    uint32_t BLCMDHardware::make_can_id(BLCMDReceiveCommand command)
    {
        return static_cast<uint32_t>(CanIdPrefix::RECEIVE) << 8 | can_id_ << 4 |
               static_cast<uint32_t>(command);
    }

    uint32_t BLCMDHardware::make_can_id(TelemetryPacket packet)
    {
        return static_cast<uint32_t>(CanIdPrefix::RECEIVE) << 8 | can_id_ << 4|
               static_cast<uint32_t>(packet);
    }

    void BLCMDHardware::packet_1_callback(org::jcan::Frame frame) {
    if(hw_velocity_.state.has_value()) hw_velocity_.state = int16_bytes_to_double(&frame.data[0]);
    if(hw_effort_.state.has_value()) hw_effort_.state = int16_bytes_to_double(&frame.data[2]);
    }

    void BLCMDHardware::packet3_callback(org::jcan::Frame frame) {
        hw_position_.state = int16_bytes_to_double(&frame.data[0]) * (hw_position_.max - hw_position_.min)/2 *
                hw_position_.resolver_reduction;
    }



    int16_t BLCMDHardware::convert_to_int16 (const double value) {
        // Convert the value to an integer
        return static_cast<int16_t>(value * 32767.0f);
    }

    int16_t BLCMDHardware::from_bytes(const uint8_t *bytes) {
        return static_cast<int16_t>(bytes[0] << 8) | static_cast<int16_t>(bytes[1]);
    }

    double BLCMDHardware::int16_bytes_to_double (uint8_t* bytes)
    {
        // Scale the value to a double
        return from_bytes(bytes)/32767.0;
    }

    double BLCMDHardware::uint16_bytes_to_double (uint8_t* bytes)
    {
        // Scale the value to a double
        return from_bytes(bytes)/65535.0;
    }

}  // namespace rover_hardware

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  rover_hardware::BLCMDHardware, hardware_interface::ActuatorInterface)
