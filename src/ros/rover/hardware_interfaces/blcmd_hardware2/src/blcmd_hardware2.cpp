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


// This file contains a bunch of hardcoded logic for the differential wrist that doesn't really belong here.
// We should find a better solution at some point. - Jackson

#include <limits>
#include <vector>
#include <chrono>
#include <cmath>
#include <sstream>
#include <algorithm>
#include <iterator>

#include "blcmd_hardware2/blcmd_hardware2.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

#include "jcan/jcan.h"

namespace blcmd_hardware
{

template<typename T>
void differential_convert_to_motors(T pitch, T yaw, T& j5, T& j6) {
  j5 = (pitch + yaw);
  j6 = (pitch - yaw);
}

template<typename T>
void differential_convert_from_motors(T j5, T j6, T& pitch, T& yaw) {
  pitch = (j5 + j6) / 2.0;
  yaw = (j5 - j6) / 2.0;
}

hardware_interface::CallbackReturn BLCMDHardware::on_init(
        const hardware_interface::HardwareInfo & info)
{
    if (hardware_interface::SystemInterface::on_init(info) != CallbackReturn::SUCCESS)
    {
      return CallbackReturn::ERROR;
    }

    BLCMDHardwareLoggerName = info_.name;
    
    auto params_result = apply_parameters();
    if (params_result != CallbackReturn::SUCCESS)
        return params_result;

    if (info_.joints.size() != params_.canids.size())
    {
      RCLCPP_FATAL_STREAM(
        rclcpp::get_logger(BLCMDHardwareLoggerName),
        "Hardware interface '" << info_.name << "got " << info_.joints.size() << " joints but expected " << params_.canids.size());
      return CallbackReturn::ERROR;
    }

    for (long unsigned int i = 0; i < params_.canids.size(); i++) {
      hw_positions_.push_back(PositionInterface{});
      hw_velocities_.push_back(ControlInterface{});
      hw_efforts_.push_back(ControlInterface{});
    }

    for (long unsigned int i = 0; i < params_.canids.size(); i++) {
        for (const auto& interface : info_.joints[i].command_interfaces){
            if(!set_control_interface(interface, true, i)){
                return CallbackReturn::ERROR;
            }
        }

        for (const auto& interface : info_.joints[i].state_interfaces){
            if(!set_control_interface(interface, false, i)){
                return CallbackReturn::ERROR;
            }
        }

    }
    
    control_mode_ = blcmd_hardware::ControlMode::Undefined;
	
    bus_ = leigh::jcan::new_bus();
    can_setup();

    return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn BLCMDHardware::on_configure(
        const rclcpp_lifecycle::State & previous_state)
{
    (void)previous_state; // Silence unused warning

    auto params_result = apply_parameters();
    if (params_result != CallbackReturn::SUCCESS)
        return params_result;

    if (params_.diff_wrist == true) {
        RCLCPP_INFO(rclcpp::get_logger(BLCMDHardwareLoggerName), "Using Diff Wrist Mode");
    }

    // open the can bus
    try {
        bus_->open(params_.candevice.c_str());
        RCLCPP_INFO(rclcpp::get_logger(BLCMDHardwareLoggerName), "Opened canbus on device %s",
                    params_.candevice.c_str());
    } catch (std::exception &e) {
        RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "Failed to start canbus with error: %s",
                     e.what());
        return CallbackReturn::ERROR;
    }


    if (!params_.mock) {
        //get min_interval
        for (long unsigned int i = 0; i < params_.canids.size(); i++) {
            if (hw_velocities_[i].state.has_value()) {
                RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                "Getting min interval on BLCMD " << params_.canids[i]);
                auto min_interval = get_config<uint16_t>(BLCMDConfigCommand::MIN_INTERVAL, params_.canids[i]);

                if (min_interval.has_value()) {
                    params_.min_interval = min_interval.value();
                    RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                    "Min interval on BLCMD " << params_.canids[i] << " is " << params_.min_interval);
                } else {
                    RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                        "Error getting min interval on BLCMD " << params_.canids[i]);
                    return CallbackReturn::ERROR;
                }
            }

            // check for resolver if there is a position interface
            if (hw_positions_[i].state.has_value() || hw_positions_[i].command.has_value()) {
                RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                "Checking for resolver on BLCMD " << params_.canids[i]);

                auto resolver_check = get_config<uint16_t>(BLCMDConfigCommand::HAS_RESOLVER, params_.canids[i]);
                if (resolver_check.has_value()) {
                    if (resolver_check.value()) {
                        RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                        "Resolver detected on BLCMD " << params_.canids[i]);
                        return CallbackReturn::SUCCESS;
                    } else {
                        RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                            "No resolver detected on BLCMD " << params_.canids[i]);
                        return CallbackReturn::ERROR;
                    }
                }
                RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                    "Error with resolver request on BLCMD" << params_.canids[i]);
                return CallbackReturn::ERROR;
            }
        }
        
    }
    
  return CallbackReturn::SUCCESS;
}

template<typename T>
std::optional<T> BLCMDHardware::get_config(BLCMDConfigCommand command, uint32_t canid) {

    const leigh::jcan::Frame min_interval_request = {
            make_can_id(BLCMDSendCommand::GET_CONFIG, canid),
            {static_cast<uint8_t>(command)},
    };
    auto start = std::chrono::steady_clock::now();
    bus_->send(min_interval_request);

    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(1)) {
        try {
            auto frame = bus_->receive_with_timeout(1000);
            if (frame.id == make_can_id(BLCMDReceiveCommand::CONFIG_DATA, canid)) {
                auto config_value = from_bytes<T>(&frame.data[0]);
                return std::optional(config_value);
            }
        } catch (std::exception &e) {
            RCLCPP_ERROR(rclcpp::get_logger(BLCMDHardwareLoggerName), "Failed to get BLCMD config with error: %s",
                         e.what());
            return std::nullopt;
        }
    }

    RCLCPP_ERROR(rclcpp::get_logger(BLCMDHardwareLoggerName), "Failed to get BLCMD config, as the received frame ID wasn't config data.");

    return std::nullopt;
}

std::vector<hardware_interface::StateInterface> BLCMDHardware::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  for (long unsigned int i = 0; i < params_.canids.size(); i++) {
    if (hw_positions_[i].state.has_value()) {
      state_interfaces.emplace_back(
              info_.joints[i].name, hardware_interface::HW_IF_POSITION, &hw_positions_[i].state.value());
    }
    if (hw_velocities_[i].state.has_value()) {
        state_interfaces.emplace_back(
                info_.joints[i].name, hardware_interface::HW_IF_VELOCITY, &hw_velocities_[i].state.value());
    }
    if (hw_efforts_[i].state.has_value()) {
        state_interfaces.emplace_back(
                info_.joints[i].name, hardware_interface::HW_IF_EFFORT, &hw_efforts_[i].state.value());
    }
  }

  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> BLCMDHardware::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;
  for (long unsigned int i = 0; i < params_.canids.size(); i++) {
        if (hw_positions_[i].command.has_value()) {
            command_interfaces.emplace_back(
                    info_.joints[i].name, hardware_interface::HW_IF_POSITION, &hw_positions_[i].command.value());
        }
        if (hw_velocities_[i].command.has_value()) {
            command_interfaces.emplace_back(
                    info_.joints[i].name, hardware_interface::HW_IF_VELOCITY, &hw_velocities_[i].command.value());
        }
        if (hw_efforts_[i].command.has_value()) {
            command_interfaces.emplace_back(
                    info_.joints[i].name, hardware_interface::HW_IF_EFFORT, &hw_efforts_[i].command.value());
        }
    }

  return command_interfaces;
}

hardware_interface::CallbackReturn BLCMDHardware::on_activate(
        const rclcpp_lifecycle::State & /*previous_state*/)
{
    bus_->set_callbacks_enabled(true);
    return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn BLCMDHardware::on_deactivate(
    const rclcpp_lifecycle::State & /*previous_state*/)
{
    bus_->set_callbacks_enabled(false);
    //TODO: halt
    return CallbackReturn::SUCCESS;
}

hardware_interface::return_type BLCMDHardware::read(
        const rclcpp::Time & /*time*/, const rclcpp::Duration & period)
{
    bus_->spin();

    for (long unsigned int i = 0; i < params_.canids.size(); i++) {
        if(params_.integrate_velocity && hw_positions_[i].state.has_value() && hw_velocities_[i].state.has_value()){
            //RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName), "Velocity state: " << hw_velocity_.state.value()
            //<< ", Position state: " << hw_position_.state.value() << ", Period: " << period.seconds());
            hw_positions_[i].state = hw_positions_[i].state.value() + hw_velocities_[i].state.value()*period.seconds();
            //RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName), "New position state: " << hw_position_.state.value());

        }
    }
    
    return hardware_interface::return_type::OK;
}

#define INT16_DISCONTINUITY_GUARD 200

hardware_interface::return_type BLCMDHardware::write(
        const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
    switch(control_mode_){
        case blcmd_hardware::ControlMode::Undefined:
            break;
        case blcmd_hardware::ControlMode::Position:
            if (params_.diff_wrist) {
                assert(hw_positions_.size() == 2 && params_.canids.size() == 2);

                double joint_rads1 = hw_positions_[0].command.value() * reversed_multiplier_;
                double resRads1 = joint_rads1 * params_.resolver_reduction;
                double resRevs1 = resRads1 / (2*M_PI);
                double resTicks1 = resRevs1 * params_.res_ticks_per_rev + params_.zero_offset;
                /// Make sure we never tell the blcmd to go close to the discontinuity around
                /// 0x7fff (largest positive) and 0x8000 (smallest negative)
                if (std::abs(resTicks1) > std::numeric_limits<int16_t>::max() - INT16_DISCONTINUITY_GUARD) {
                    RCLCPP_WARN_THROTTLE(rclcpp::get_logger(BLCMDHardwareLoggerName), *get_clock(), 2000,
                    "Position Command %f is too close to/beyond the int16 discontinuity. Ignoring...", resTicks1);
                    return hardware_interface::return_type::OK;
                }

                double joint_rads2 = hw_positions_[1].command.value() * reversed_multiplier_;
                double resRads2 = joint_rads2 * params_.resolver_reduction;
                double resRevs2 = resRads2 / (2*M_PI);
                double resTicks2 = resRevs2 * params_.res_ticks_per_rev + params_.zero_offset;
                /// Make sure we never tell the blcmd to go close to the discontinuity around
                /// 0x7fff (largest positive) and 0x8000 (smallest negative)
                if (std::abs(resTicks1) > std::numeric_limits<int16_t>::max() - INT16_DISCONTINUITY_GUARD) {
                    RCLCPP_WARN_THROTTLE(rclcpp::get_logger(BLCMDHardwareLoggerName), *get_clock(), 2000,
                    "Position Command %f is too close to/beyond the int16 discontinuity. Ignoring...", resTicks2);
                    return hardware_interface::return_type::OK;
                }

                auto offset_value1 = resTicks1;
                auto offset_value2 = resTicks2;
                double value1, value2;
                differential_convert_to_motors(offset_value1, offset_value2, value1, value2);

                RCLCPP_DEBUG_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                "Sending Diff Wrist Position Command ");
                RCLCPP_INFO(rclcpp::get_logger(BLCMDHardwareLoggerName), "Diff wrist Position");
                send_scaled<int16_t>(make_can_id(BLCMDSendCommand::DRIVE_POSITION, params_.canids[0]),
                                    value1, hw_positions_[0].max);
                send_scaled<int16_t>(make_can_id(BLCMDSendCommand::DRIVE_POSITION, params_.canids[1]),
                                    value2, hw_positions_[1].max);

                break;
            }
            for (long unsigned int i = 0; i < params_.canids.size(); i++) {
                if (hw_positions_[i].command.has_value()) {

                    double joint_rads = hw_positions_[i].command.value() * reversed_multiplier_;
                    double resRads = joint_rads * params_.resolver_reduction;
                    double resRevs = resRads / (2*M_PI);
                    double resTicks = resRevs * params_.res_ticks_per_rev + params_.zero_offset;
                    /// Make sure we never tell the blcmd to go close to the discontinuity around
                    /// 0x7fff (largest positive) and 0x8000 (smallest negative)
                    if (std::abs(resTicks) > std::numeric_limits<int16_t>::max() - INT16_DISCONTINUITY_GUARD) {
                      RCLCPP_WARN_THROTTLE(rclcpp::get_logger(BLCMDHardwareLoggerName), *get_clock(), 2000,
                        "Position Command %f is too close to/beyond the int16 discontinuity. Ignoring...", resTicks);
                      return hardware_interface::return_type::OK;
                    }

                    RCLCPP_DEBUG_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                    "Sending Position Command " << hw_positions_[i].command.value()
                                    << " " << hw_positions_[i].max);
                    send_raw<int16_t>(make_can_id(BLCMDSendCommand::DRIVE_POSITION, params_.canids[i]),
                        static_cast<int16_t>(resTicks));
                } else {
                    RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No position command");
                    return hardware_interface::return_type::ERROR;
                }
            }
            break;
        case blcmd_hardware::ControlMode::Velocity:
            if (params_.diff_wrist) {
                assert(hw_velocities_.size() == 2 && params_.canids.size() == 2);

                double value1, value2;
                differential_convert_to_motors(hw_velocities_[0].command.value(), hw_velocities_[1].command.value(), value1, value2);

                RCLCPP_DEBUG_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                    "Sending diff wrist velocity command");

                send_scaled<int16_t>(make_can_id(BLCMDSendCommand::DRIVE_VELOCITY, params_.canids[0]),
                                    value1 * reversed_multiplier_, hw_velocities_[0].max);
                send_scaled<int16_t>(make_can_id(BLCMDSendCommand::DRIVE_VELOCITY, params_.canids[1]),
                                    value2 * reversed_multiplier_, hw_velocities_[1].max);
                break;
            }

            for (long unsigned int i = 0; i < params_.canids.size(); i++) {
                if (hw_velocities_[i].command.has_value()) {
                RCLCPP_DEBUG_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                    "Sending velocity command " << hw_velocities_[i].command.value() * reversed_multiplier_
                                    << " " << hw_velocities_[i].max);
                    send_scaled<int16_t>(make_can_id(BLCMDSendCommand::DRIVE_VELOCITY, params_.canids[i]),
                                        hw_velocities_[i].command.value() * reversed_multiplier_, hw_velocities_[i].max);
                } else {
                    RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No velocity command");
                    return hardware_interface::return_type::ERROR;
                }
            }
            break;
        case blcmd_hardware::ControlMode::Effort:
            if (params_.diff_wrist) {
                assert(hw_efforts_.size() == 2 && params_.canids.size() == 2);

                double value1, value2;
                differential_convert_to_motors(hw_efforts_[0].command.value(), hw_efforts_[1].command.value(), value1, value2);

                RCLCPP_DEBUG_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                    "Sending diff wrist velocity command");                

                send_scaled<int16_t>(make_can_id(BLCMDSendCommand::DRIVE_VELOCITY, params_.canids[0]),
                                    value1 * reversed_multiplier_, hw_efforts_[0].max);
                send_scaled<int16_t>(make_can_id(BLCMDSendCommand::DRIVE_VELOCITY, params_.canids[1]),
                                    value2 * reversed_multiplier_, hw_efforts_[1].max);
                break;
            }

            for (long unsigned int i = 0; i < params_.canids.size(); i++) {
                if (hw_efforts_[i].command.has_value()) {
                    send_scaled<int16_t>(make_can_id(BLCMDSendCommand::DRIVE_CURRENT, params_.canids[i]),
                                        hw_efforts_[i].command.value() * reversed_multiplier_, hw_efforts_[i].max);
                } else {
                    RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No effort command");
                    return hardware_interface::return_type::ERROR;
                }
            }
            break;
    }
    return hardware_interface::return_type::OK;
}

hardware_interface::return_type
    BLCMDHardware::prepare_command_mode_switch(const std::vector<std::string> &start_interfaces,
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

bool BLCMDHardware::stop_interface(const std::string &interface){

    size_t delimiter_pos = interface.find('/');

    std::string joint_name = interface.substr(0, delimiter_pos);
    std::string interface_name = interface.substr(delimiter_pos + 1);

    if (joint_name != info_.joints[0].name){
        return true;
    }

    if (interface_name == hardware_interface::HW_IF_POSITION) {
        if (control_mode_ == blcmd_hardware::ControlMode::Position) {
            control_mode_ = blcmd_hardware::ControlMode::Undefined;
            return true;
        } else {
            RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                "Requested stop position when control mode was not position");
            return false;
        }
    }
    if (interface_name == hardware_interface::HW_IF_VELOCITY) {
        if (control_mode_ == blcmd_hardware::ControlMode::Velocity) {
            for (auto& hw_velocity_ : hw_velocities_)
                hw_velocity_.command = 0.0;
            control_mode_ = blcmd_hardware::ControlMode::Undefined;
            return true;
        } else {
            RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                "Requested stop " << interface_name.c_str() << "when control mode was not velocity");
            return false;
        }
    }
    if (interface_name == hardware_interface::HW_IF_EFFORT) {
        if (control_mode_ == blcmd_hardware::ControlMode::Effort) {
            for (auto& hw_effort_ : hw_efforts_)
                hw_effort_.command = 0.0;
            control_mode_ = blcmd_hardware::ControlMode::Undefined;
            return true;
        } else {
            RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                "Requested stop " << interface_name.c_str() << "when control mode was not effort");
            return false;
        }
    }

    RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                        "Unexpected interface " << interface_name.c_str());
    return false;

}

bool BLCMDHardware::start_interface(const std::string &interface){

    size_t delimiter_pos = interface.find('/');

    std::string joint_name = interface.substr(0, delimiter_pos);
    std::string interface_name = interface.substr(delimiter_pos + 1);

    if (joint_name != info_.joints[0].name){
        return true;
    }

    if(control_mode_ != blcmd_hardware::ControlMode::Undefined){
        RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                            "Requested start interface " << interface.c_str() << " when control mode was not undefined");
        return false;
    }

    if (interface_name == hardware_interface::HW_IF_POSITION){
        control_mode_ = blcmd_hardware::ControlMode::Position;
    } else if (interface_name == hardware_interface::HW_IF_VELOCITY){
        control_mode_ = blcmd_hardware::ControlMode::Velocity;
    } else if (interface_name == hardware_interface::HW_IF_EFFORT){
        control_mode_ = blcmd_hardware::ControlMode::Effort;
    } else {
        RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                            "Unexpected interface " << interface_name.c_str());
        return false;
    }

    return true;
}

hardware_interface::CallbackReturn BLCMDHardware::apply_parameters() {
  auto canbus_search = info_.hardware_parameters.find("candevice");
  if (canbus_search == info_.hardware_parameters.end()){
    RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No canbus provided");
    return CallbackReturn::ERROR;
  }
  params_.candevice = canbus_search->second;
  RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                     "Using can device " << params_.candevice.c_str());

  auto canid_search = info_.hardware_parameters.find("canid");
  if (canid_search == info_.hardware_parameters.end()){
    RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No canid provided");
    return CallbackReturn::ERROR;
  }
  std::stringstream sstream(canid_search->second);
  std::string current_word;
  params_.canids = {};

  while (sstream >> current_word) {
    params_.canids.push_back(std::stoul(current_word));
  }

  RCLCPP_INFO(rclcpp::get_logger(BLCMDHardwareLoggerName), "Using can ids %s", canid_search->second.c_str());

  auto clock_rate_search = info_.hardware_parameters.find("clock_rate");
  if (clock_rate_search == info_.hardware_parameters.end()){
    RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No clock rate provided");
    return CallbackReturn::ERROR;
  }
  params_.clock_rate = std::stoul(clock_rate_search->second);
  RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                     "Got clock rate: " << params_.clock_rate);

  auto revolution_pulses_search = info_.hardware_parameters.find("revolution_pulses");
  if (revolution_pulses_search == info_.hardware_parameters.end()){
    RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No revolution pulses provided");
    return CallbackReturn::ERROR;
  }
  params_.revolution_pulses = std::stoul(revolution_pulses_search->second);
  RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                     "Resolver pulses: " << params_.revolution_pulses);

  auto mock_search = info_.hardware_parameters.find("mock");
  if (mock_search != info_.hardware_parameters.end()) {
    params_.mock = is_true(mock_search->second);
  }

  auto reversed_search = info_.hardware_parameters.find("reversed");
  if (reversed_search != info_.hardware_parameters.end() && is_true(reversed_search->second)) {
    RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                       "Interface is reversed");
    params_.reversed = true;
    reversed_multiplier_ = -1;
  }

  auto integrate_velocity_search = info_.hardware_parameters.find("integrate_velocity");
  if (integrate_velocity_search != info_.hardware_parameters.end() && is_true(integrate_velocity_search->second)) {
    RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                       "Integrating velocity to provide position estimate");
    params_.integrate_velocity = true;
  }

  auto min_interval_search = info_.hardware_parameters.find("min_interval");
  if (min_interval_search != info_.hardware_parameters.end() && params_.mock){
    params_.min_interval = std::stol(min_interval_search->second);
  }

  auto gear_ratio_search = info_.hardware_parameters.find("gear_ratio");
  if (gear_ratio_search == info_.hardware_parameters.end()) {
    RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No gear ratio provided");
    return CallbackReturn::ERROR;
  }
  params_.gear_ratio = std::stod(gear_ratio_search->second);
  RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                     "Got gear ratio: " << params_.gear_ratio);

  auto max_velocity_search = info_.hardware_parameters.find("max_velocity");
  if (max_velocity_search != info_.hardware_parameters.end()) {
    params_.max_velocity = std::stod(max_velocity_search->second);
    RCLCPP_INFO(rclcpp::get_logger(BLCMDHardwareLoggerName), "Using max_velocity of %f", params_.max_velocity.value());
  }
  else {
    RCLCPP_INFO(rclcpp::get_logger(BLCMDHardwareLoggerName), "max_velocity parameter was undefined. Will use "
                                                             "complex formula to make this value.");
  }

  auto resolver_reduction_search = info_.hardware_parameters.find("resolver_reduction");
  if (resolver_reduction_search != info_.hardware_parameters.end()) {
    params_.resolver_reduction = std::stod(resolver_reduction_search->second);
  }
  
  auto zero_offset_search = info_.hardware_parameters.find("zero_offset");
  if (zero_offset_search != info_.hardware_parameters.end()) {
    params_.zero_offset = std::stoul(zero_offset_search->second);

    RCLCPP_INFO(rclcpp::get_logger(BLCMDHardwareLoggerName), "Using zero_offset of: 0x%x", params_.zero_offset);
  }

  auto res_ticks_per_rev_search = info_.hardware_parameters.find("res_ticks_per_rev");
  if (res_ticks_per_rev_search != info_.hardware_parameters.end()) {
    params_.res_ticks_per_rev = std::stoul(res_ticks_per_rev_search->second);
    RCLCPP_INFO(rclcpp::get_logger(BLCMDHardwareLoggerName), "Using 0x%x ticks per absolute encoder revolution", params_.res_ticks_per_rev);
  }

  auto diff_wrist_search = info_.hardware_parameters.find("diff_wrist");
  if (diff_wrist_search != info_.hardware_parameters.end() && is_true(diff_wrist_search->second)) {
    RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                    "Diff Wrist Enabled");
    params_.diff_wrist= true;
  }

  return CallbackReturn::SUCCESS;
}

// TODO: better error handling
bool BLCMDHardware::set_control_interface(
        const hardware_interface::InterfaceInfo &interface_info, bool command, int index) {
    if (interface_info.name == hardware_interface::HW_IF_POSITION){
        // TODO: deal with case with state interface and no command interface
        if (command){
              if (params_.res_ticks_per_rev==0) {
                  RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No resolver ticks per rev provided, "
                                                                          "but a position command interface is used.");
                  return false;
              }
              if (std::isnan(params_.resolver_reduction)) {
                  RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No resolver reduction provided, "
                                                                          "but a position command interface is used.");
                  return false;
              }
              //TODO: this isn't actually correct as if the zero offset is non zero there will be a different max and min.
              hw_positions_[index].max = 2 * M_PI * (0x7fff-INT16_DISCONTINUITY_GUARD) / (params_.res_ticks_per_rev*params_.resolver_reduction);
              hw_positions_[index].command = 0.0;
            
        } else {
            hw_positions_[index].state = 0.0;
        }
    }
    else if (interface_info.name == hardware_interface::HW_IF_VELOCITY){
        if (command) {
            hw_velocities_[index].max = params_.max_velocity.has_value() ? params_.max_velocity.value()              
            : (params_.clock_rate) / (params_.min_interval * params_.revolution_pulses * params_.gear_ratio) * 2 * M_PI;
            RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
            "Configured velocity interface with max velocity: " << hw_velocities_[index].max);
            hw_velocities_[index].command = 0.0;
    } else {
            hw_velocities_[index].state = 0.0;
        }
    }
    else if (interface_info.name == hardware_interface::HW_IF_EFFORT){
        if (command){
            hw_efforts_[index].max = std::stod(interface_info.max);;
            hw_efforts_[index].command = 0.0;
        } else {
            hw_efforts_[index].state = 0.0;
        }
    } else {
        RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "Unexpected interface %s",
                     interface_info.name.c_str());
        return false;
    }
    return true;
}

    void BLCMDHardware::can_setup() {
        std::vector<uint32_t> ids = {};
        for (long unsigned int i = 0; i < params_.canids.size(); i++) {
            ids.push_back(make_can_id(BLCMDReceiveCommand::CONFIG_DATA, params_.canids[i]));
            if (hw_velocities_[i].state.has_value() || hw_efforts_[i].state.has_value()) {
                ids.push_back(make_can_id(TelemetryPacket::PACKET_1, params_.canids[i]));
            }
            if (hw_positions_[i].state.has_value() && !params_.integrate_velocity) {
                ids.push_back(make_can_id(TelemetryPacket::PACKET_3, params_.canids[i]));
            }
        }
	      bus_->set_id_filter(ids);
        for (long unsigned int i = 0; i < params_.canids.size(); i++) {
            if (hw_velocities_[i].state.has_value() || hw_efforts_[i].state.has_value()) {
                RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                "Adding packet 1 callback to ID:" << make_can_id(TelemetryPacket::PACKET_1, params_.canids[i]));
                bus_->add_callback_to(make_can_id(TelemetryPacket::PACKET_1, params_.canids[i]), this, &BLCMDHardware::packet_1_callback);
            }
            if (hw_positions_[i].state.has_value() && !params_.integrate_velocity) {
                RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                "Adding packet 3 callback to ID:" << make_can_id(TelemetryPacket::PACKET_3, params_.canids[i]));
                bus_->add_callback_to(make_can_id(TelemetryPacket::PACKET_3, params_.canids[i]), this, &BLCMDHardware::packet_3_callback);
            }
        }
        bus_->set_callbacks_enabled(false);
   }

    uint32_t BLCMDHardware::make_can_id(BLCMDSendCommand command, uint32_t canid) const
    {
        return static_cast<uint32_t>(CanIdPrefix::SEND) << 8 | canid << 4 |
               static_cast<uint32_t>(command);
    }

    uint32_t BLCMDHardware::make_can_id(BLCMDReceiveCommand command, uint32_t canid) const
    {
        return static_cast<uint32_t>(CanIdPrefix::RECEIVE) << 8 | canid << 4 |
               static_cast<uint32_t>(command);
    }

    uint32_t BLCMDHardware::make_can_id(TelemetryPacket packet, uint32_t canid) const
    {
        return static_cast<uint32_t>(CanIdPrefix::RECEIVE) << 8 | canid << 4|
               static_cast<uint32_t>(packet);
    }

    void BLCMDHardware::packet_1_callback(leigh::jcan::Frame frame) {
        auto canid = (frame.id & 0x0f0) >> 4;
        auto id_location = std::find(params_.canids.begin(), params_.canids.end(), canid);

        if (id_location == params_.canids.end()) {
          RCLCPP_ERROR(rclcpp::get_logger(BLCMDHardwareLoggerName), "%s: %d is not in our canid list.", __func__, canid);
          return;
        }

        auto i = std::distance(params_.canids.begin(), id_location);
        if(hw_velocities_.at(i).state.has_value()) {
            if (params_.diff_wrist && i == 0) {
               differential_actual_value1 = convert_scaled<int16_t>(&frame.data[0], hw_velocities_.at(i).max) *
                 reversed_multiplier_*-1*0.5; // Dear Bro, ask chassis why this is -1
                                              //
                double converted_value1, converted_value2;
                differential_convert_from_motors(differential_actual_value1, differential_actual_value2, converted_value1, converted_value2);
               hw_velocities_.at(0).state = converted_value1;
               hw_velocities_.at(1).state = converted_value2;
            } else if (params_.diff_wrist && i == 1) {
               differential_actual_value2 = convert_scaled<int16_t>(&frame.data[0], hw_velocities_.at(i).max) *
                 reversed_multiplier_*-1*0.5; // Dear Bro, ask chassis why this is -1
                                              //
                double converted_value1, converted_value2;
                differential_convert_from_motors(differential_actual_value1, differential_actual_value2, converted_value1, converted_value2);
               hw_velocities_.at(0).state = converted_value1;
               hw_velocities_.at(1).state = converted_value2;
            } else {
              hw_velocities_.at(i).state = convert_scaled<int16_t>(&frame.data[0], hw_velocities_.at(i).max) *
              reversed_multiplier_*-1*0.5; // Dear Bro, ask chassis why this is -1
            }
        }

        if(hw_efforts_.at(i).state.has_value()) {
             if (params_.diff_wrist && i == 0) {
               differential_actual_value1 = convert_scaled<int16_t>(&frame.data[0], hw_efforts_.at(i).max);
                double converted_value1, converted_value2;
                differential_convert_from_motors(differential_actual_value1, differential_actual_value2, converted_value1, converted_value2);
                hw_efforts_.at(0).state = converted_value1;
                hw_efforts_.at(1).state = converted_value2;
            } else if (params_.diff_wrist && i == 1) {
               differential_actual_value2 = convert_scaled<int16_t>(&frame.data[0], hw_efforts_.at(i).max);
                double converted_value1, converted_value2;
                differential_convert_from_motors(differential_actual_value1, differential_actual_value2, converted_value1, converted_value2);
                hw_efforts_.at(0).state = converted_value1;
                hw_efforts_.at(1).state = converted_value2;
            } else {
              hw_efforts_.at(i).state = convert_scaled<int16_t>(&frame.data[2], hw_efforts_.at(i).max);
            }
        }
    }

    void BLCMDHardware::packet_3_callback(leigh::jcan::Frame frame) {
        auto canid = (frame.id & 0x0f0) >> 4;
        auto id_location = std::find(params_.canids.begin(), params_.canids.end(), canid);

        if (id_location == params_.canids.end()) {
          RCLCPP_ERROR(rclcpp::get_logger(BLCMDHardwareLoggerName), "%s: %d is not in our canid list.", __func__, canid);
          return;
        }

        auto i = std::distance(params_.canids.begin(), id_location);

        if(hw_positions_.at(i).state.has_value()) {
            if (params_.diff_wrist && i == 0) {
                const Telem3_t *telem3;

                if (frame.data.size() != sizeof(Telem3_t)) {
                    RCLCPP_WARN(rclcpp::get_logger(BLCMDHardwareLoggerName), "Telemetry 3 Incorrect size: %ld", frame.data.size());
                    hw_positions_.at(i).state = std::numeric_limits<double>::quiet_NaN();
                    return;
                }

                telem3 = (const Telem3_t*)frame.data.data();

                int32_t resolverPosTicks = beToCPU16(telem3->resPosition) - params_.zero_offset;
                double resolverPosRevs = static_cast<double>(resolverPosTicks) / params_.res_ticks_per_rev;
                double resolverPosRads = 2*M_PI*resolverPosRevs;
                double jointPosRads = resolverPosRads * reversed_multiplier_ / params_.resolver_reduction;

                differential_actual_value1 = jointPosRads;
                double converted_value1, converted_value2;
                differential_convert_from_motors(differential_actual_value1, differential_actual_value2, converted_value1, converted_value2);
                hw_positions_.at(0).state = converted_value1;
                hw_positions_.at(1).state = converted_value2;
            } else if (params_.diff_wrist && i == 1) {
                const Telem3_t *telem3;

                if (frame.data.size() != sizeof(Telem3_t)) {
                    RCLCPP_WARN(rclcpp::get_logger(BLCMDHardwareLoggerName), "Telemetry 3 Incorrect size: %ld", frame.data.size());
                    hw_positions_.at(i).state = std::numeric_limits<double>::quiet_NaN();
                    return;
                }

                telem3 = (const Telem3_t*)frame.data.data();

                int32_t resolverPosTicks = beToCPU16(telem3->resPosition) - params_.zero_offset;
                double resolverPosRevs = static_cast<double>(resolverPosTicks) / params_.res_ticks_per_rev;
                double resolverPosRads = 2*M_PI*resolverPosRevs;
                double jointPosRads = resolverPosRads * reversed_multiplier_ / params_.resolver_reduction;

                differential_actual_value2 = jointPosRads;
                double converted_value1, converted_value2;
                differential_convert_from_motors(differential_actual_value1, differential_actual_value2, converted_value1, converted_value2);
                hw_positions_.at(0).state = converted_value1;
                hw_positions_.at(1).state = converted_value2;
            } else {
                const Telem3_t *telem3;

                if (frame.data.size() != sizeof(Telem3_t)) {
                    RCLCPP_WARN(rclcpp::get_logger(BLCMDHardwareLoggerName), "Telemetry 3 Incorrect size: %ld", frame.data.size());
                    hw_positions_.at(i).state = std::numeric_limits<double>::quiet_NaN();
                    return;
                }

                telem3 = (const Telem3_t*)frame.data.data();

                int32_t resolverPosTicks = beToCPU16(telem3->resPosition) - params_.zero_offset;
                double resolverPosRevs = static_cast<double>(resolverPosTicks) / params_.res_ticks_per_rev;
                double resolverPosRads = 2*M_PI*resolverPosRevs;
                double jointPosRads = resolverPosRads * reversed_multiplier_ / params_.resolver_reduction;

                hw_positions_.at(i).state = jointPosRads;
            }
        }
    }

  template<typename T>
    double BLCMDHardware::convert_scaled(const uint8_t *bytes, double max) {
        return static_cast<double>(from_bytes<T>(bytes))/ std::numeric_limits<T>::max() * max;
    }

    template<typename T>
    T BLCMDHardware::from_bytes(const uint8_t *bytes) {
        T data = bytes[0];
        for(unsigned int i = 1; i < sizeof(T); i++) {
            data = data << 8 | bytes[i];
        }
        return data;
    }

    template<typename T>
    void BLCMDHardware::send_scaled(uint32_t id, double value, double max) {
        // note: anything less than 2 domains left will not be correct
        auto one_clamped_scalar = std::fmod((value/max) + 3, 2) - 1;


        // (abs(value) > max ? (value > 0 ? 1 : -1) : value/max

        T data = static_cast<T>(one_clamped_scalar * std::numeric_limits<T>::max());
        send_raw(id, data);
    }

    template<typename T>
    void BLCMDHardware::send_raw(const uint32_t id, T data) {
        leigh::jcan::Frame frame;
        frame.id = id;
        for(unsigned int i = 0; i < sizeof(T); i++) {
            frame.data.push_back(data >> 8*(sizeof(T) - (i + 1)) & 0xFF);
        }
        bus_->send(frame);
    }

    bool BLCMDHardware::is_true(std::string& text) {
        return text == "true" || text == "True";
    }
}  // namespace blcmd_hardware

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  blcmd_hardware::BLCMDHardware, hardware_interface::SystemInterface)
