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
#include <chrono>
#include <cmath>

#include "auger_hardware/auger_hardware.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

#include "jcan/jcan.h"

namespace auger_hardware
{
hardware_interface::CallbackReturn AugerHardware::on_init(
        const hardware_interface::HardwareInfo & info)
{
    if (hardware_interface::SystemInterface::on_init(info) != CallbackReturn::SUCCESS)
    {
      return CallbackReturn::ERROR;
    }

    AugerHardwareLoggerName = info_.name;

    if (info_.joints.size() != 1)
    {
      RCLCPP_FATAL_STREAM(
        rclcpp::get_logger(AugerHardwareLoggerName),
        "Hardware interface '" << info_.name << "got " << info_.joints.size() << " joints but expected 1");
      return CallbackReturn::ERROR;
    }

    auto canbus_search = info_.hardware_parameters.find("candevice");
    if (canbus_search == info_.hardware_parameters.end()){
        RCLCPP_FATAL(rclcpp::get_logger(AugerHardwareLoggerName), "No canbus provided");
        return CallbackReturn::ERROR;
    }

    can_device_ = canbus_search->second;
    RCLCPP_INFO_STREAM(rclcpp::get_logger(AugerHardwareLoggerName),
                "Using can device " << can_device_.c_str());

    auto canid_search = info_.hardware_parameters.find("canid");
    if (canid_search == info_.hardware_parameters.end()){
        RCLCPP_FATAL(rclcpp::get_logger(AugerHardwareLoggerName), "No canid provided");
        return CallbackReturn::ERROR;
    }

    can_id_ = std::stoul(canid_search->second);
    
    RCLCPP_INFO(rclcpp::get_logger(AugerHardwareLoggerName), "Using can id %d", can_id_);

    auto clock_rate_search = info_.hardware_parameters.find("clock_rate");
    if (clock_rate_search == info_.hardware_parameters.end()){
        RCLCPP_FATAL(rclcpp::get_logger(AugerHardwareLoggerName), "No clock rate provided");
        return CallbackReturn::ERROR;
    }
    clock_rate_ = std::stoul(clock_rate_search->second);
    RCLCPP_INFO_STREAM(rclcpp::get_logger(AugerHardwareLoggerName),
                       "Got clock rate: " << clock_rate_);

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

    control_mode_ = auger_hardware::ControlMode::Undefined;

    bus_ = leigh::jcan::new_bus();
    can_setup();

    return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn AugerHardware::on_configure(
        const rclcpp_lifecycle::State & previous_state)
{
  // open the can bus
    try {
        bus_->open(can_device_.c_str());
        RCLCPP_INFO(rclcpp::get_logger(AugerHardwareLoggerName), "Opened canbus on device %s",
                    can_device_.c_str());
    } catch (std::exception &e) {
        RCLCPP_FATAL(rclcpp::get_logger(AugerHardwareLoggerName), "Failed to start canbus with error: %s",
                     e.what());
        return CallbackReturn::ERROR;
    }
    
  return CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> AugerHardware::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  if (hw_effort_.state.has_value()) {
      state_interfaces.emplace_back(
              info_.joints[0].name, hardware_interface::HW_IF_EFFORT, &hw_effort_.state.value());
  }

  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> AugerHardware::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;
    if (hw_effort_.command.has_value()) {
        command_interfaces.emplace_back(
                info_.joints[0].name, hardware_interface::HW_IF_EFFORT, &hw_effort_.command.value());
    }

  return command_interfaces;
}

hardware_interface::CallbackReturn AugerHardware::on_activate(
        const rclcpp_lifecycle::State & /*previous_state*/)
{
    bus_->set_callbacks_enabled(true);
    return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn AugerHardware::on_deactivate(
    const rclcpp_lifecycle::State & /*previous_state*/)
{
    bus_->set_callbacks_enabled(false);
    return CallbackReturn::SUCCESS;
}

hardware_interface::return_type AugerHardware::read(
        const rclcpp::Time & time, const rclcpp::Duration & period)
{
    bus_->spin();
    return hardware_interface::return_type::OK;
}

hardware_interface::return_type AugerHardware::write(
        const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
    switch(control_mode_){
        case auger_hardware::ControlMode::Undefined:
            break;
        case auger_hardware::ControlMode::Effort:
            if (hw_effort_.command.has_value()) {
                send_scaled<int16_t>(can_id_,
                                     hw_effort_.command.value(), hw_effort_.max);
            } else {
                RCLCPP_FATAL(rclcpp::get_logger(AugerHardwareLoggerName), "No effort command");
                return hardware_interface::return_type::ERROR;
            }
            break;
    }
    return hardware_interface::return_type::OK;
}

hardware_interface::return_type
    AugerHardware::prepare_command_mode_switch(const std::vector<std::string> &start_interfaces,
                                               const std::vector<std::string> &stop_interfaces) {
   for (const auto& interface : stop_interfaces) {
       if (!stop_interface(interface)) {
           return hardware_interface::return_type::ERROR;
       }
   }

   for (const auto& interface : start_interfaces) {
       if (!start_interface(interface)) {
           return hardware_interface::return_type::ERROR;
       }
   }

    return hardware_interface::return_type::OK;
}

bool AugerHardware::stop_interface(const std::string &interface){

    size_t delimiter_pos = interface.find('/');

    std::string joint_name = interface.substr(0, delimiter_pos);
    std::string interface_name = interface.substr(delimiter_pos + 1);

    if (joint_name != info_.joints[0].name){
        return true;
    }

    if (interface_name == hardware_interface::HW_IF_EFFORT) {
        if (control_mode_ == auger_hardware::ControlMode::Effort) {
            hw_effort_.command = 0.0;
            control_mode_ = auger_hardware::ControlMode::Undefined;
            return true;
        } else {
            RCLCPP_FATAL_STREAM(rclcpp::get_logger(AugerHardwareLoggerName),
                                "Requested stop " << interface_name.c_str() << "when control mode was not effort");
            return false;
        }
    }

    RCLCPP_FATAL_STREAM(rclcpp::get_logger(AugerHardwareLoggerName),
                        "Unexpected interface " << interface_name.c_str());
    return false;

}

bool AugerHardware::start_interface(const std::string &interface){

    size_t delimiter_pos = interface.find('/');

    std::string joint_name = interface.substr(0, delimiter_pos);
    std::string interface_name = interface.substr(delimiter_pos + 1);

    if (joint_name != info_.joints[0].name){
        return true;
    }

    if(control_mode_ != auger_hardware::ControlMode::Undefined){
        RCLCPP_FATAL_STREAM(rclcpp::get_logger(AugerHardwareLoggerName),
                            "Requested start interface " << interface.c_str() << " when control mode was not undefined");
        return false;
    }

    if (interface_name == hardware_interface::HW_IF_EFFORT){
        control_mode_ = auger_hardware::ControlMode::Effort;
    } else {
        RCLCPP_FATAL_STREAM(rclcpp::get_logger(AugerHardwareLoggerName),
                            "Unexpected interface " << interface_name.c_str());
        return false;
    }

    return true;
}

// TODO: better error handling
bool AugerHardware::set_control_interface(
        const hardware_interface::InterfaceInfo &interface_info, bool command) {
    if(interface_info.name == hardware_interface::HW_IF_EFFORT){
        if(command){
            hw_effort_.max = std::stod(interface_info.max);;
            hw_effort_.command = 0.0;
        } else {
            hw_effort_.state = 0.0;
        }
    } else {
        RCLCPP_FATAL(rclcpp::get_logger(AugerHardwareLoggerName), "Unexpected interface %s",
                     interface_info.name.c_str());
        return false;
    }
    return true;
}

void AugerHardware::can_setup() {
    std::vector<uint32_t> ids = {
	can_id_
    };

    bus_->set_id_filter(ids);
    if (hw_effort_.state.has_value()) {
        RCLCPP_INFO_STREAM(rclcpp::get_logger(AugerHardwareLoggerName),
                "Adding packet 1 callback to ID: " << can_id_);
        bus_->add_callback_to(static_cast<int>(can_id_), this, &AugerHardware::packet_callback);
    }
    bus_->set_callbacks_enabled(false);
}

void AugerHardware::packet_callback(leigh::jcan::Frame frame) {
    if(hw_effort_.state.has_value()) {
        hw_effort_.state = convert_scaled<int16_t>(&frame.data[2], hw_effort_.max);
    }
}

template<typename T>
double AugerHardware::convert_scaled(const uint8_t *bytes, double max) {
    return static_cast<double>(from_bytes<T>(bytes))/ std::numeric_limits<T>::max() * max;
}

template<typename T>
T AugerHardware::from_bytes(const uint8_t *bytes) {
    T data = bytes[0];
    for(unsigned int i = 1; i < sizeof(T); i++) {
        data = data << 8 | bytes[i];
    }
    return data;
}

template<typename T>
void AugerHardware::send_scaled(uint32_t id, double value, double max) {
    T data = static_cast<T>( (abs(value) > max ? (value > 0 ? 1 : -1) : value/max)* std::numeric_limits<T>::max());
    send_raw(id, data);
}

template<typename T>
void AugerHardware::send_raw(const uint32_t id, T data) {
    leigh::jcan::Frame frame;
    frame.id = id;
    for(unsigned int i = 0; i < sizeof(T); i++) {
        frame.data.push_back(data >> 8*(sizeof(T) - (i + 1)) & 0xFF);
    }
    bus_->send(frame);
}

}  // namespace auger_hardware

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  auger_hardware::AugerHardware, hardware_interface::SystemInterface)
