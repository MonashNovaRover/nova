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

#include "blcmd_hardware/blcmd_hardware.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

#include "jcan/jcan.h"

namespace blcmd_hardware
{
hardware_interface::CallbackReturn BLCMDHardware::on_init(
        const hardware_interface::HardwareInfo & info)
{
    if (hardware_interface::ActuatorInterface::on_init(info) != CallbackReturn::SUCCESS)
    {
      return CallbackReturn::ERROR;
    }

    BLCMDHardwareLoggerName = info_.name;

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

    can_id_ = std::stoul(canid_search->second);
    RCLCPP_INFO(rclcpp::get_logger(BLCMDHardwareLoggerName), "Using can id %d", can_id_);

    auto clock_rate_search = info_.hardware_parameters.find("clock_rate");
    if (clock_rate_search == info_.hardware_parameters.end()){
        RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No clock rate provided");
        return CallbackReturn::ERROR;
    }

    clock_rate_ = std::stoul(clock_rate_search->second);

    auto revolution_pulses_search = info_.hardware_parameters.find("revolution_pulses");

    if (revolution_pulses_search == info_.hardware_parameters.end()){
        RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No revolution pulses provided");
        return CallbackReturn::ERROR;
    }

    auto mock_search = info_.hardware_parameters.find("mock");
    if (mock_search != info_.hardware_parameters.end() && mock_search->second == "true"){
        mock_ = true;
    }

    revolution_pulses_ = std::stoul(revolution_pulses_search->second);


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

    control_mode_ = blcmd_hardware::ControlMode::Undefined;
	
    bus_ = leigh::jcan::new_bus();
    can_setup();

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


    if (!mock_) {
        //get min_interval
        if (hw_velocity_.state.has_value()) {
            RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                               "Getting min interval on BLCMD " << can_id_);
            auto min_interval = get_config<uint16_t>(BLCMDConfigCommand::MIN_INTERVAL);

            if (min_interval.has_value()) {
                min_interval_ = min_interval.value();
                RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                   "Min interval on BLCMD " << can_id_ << " is " << min_interval_);
            } else {
                RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                    "Error getting min interval on BLCMD " << can_id_);
                return CallbackReturn::ERROR;
            }
        }

        // check for resolver if there is a position interface
        if (hw_position_.state.has_value() || hw_position_.command.has_value()) {
            RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                               "Checking for resolver on BLCMD " << can_id_);

            auto resolver_check = get_config<uint16_t>(BLCMDConfigCommand::HAS_RESOLVER);
            if (resolver_check.has_value()) {
                if (resolver_check.value()) {
                    RCLCPP_INFO_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                       "Resolver detected on BLCMD " << can_id_);
                    return CallbackReturn::SUCCESS;
                } else {
                    RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                        "No resolver detected on BLCMD " << can_id_);
                    return CallbackReturn::ERROR;
                }
            }
            RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                                "Error with resolver request on BLCMD" << can_id_);
            return CallbackReturn::ERROR;

        }
    }
    
  return CallbackReturn::SUCCESS;
}

template<typename T>
std::optional<T> BLCMDHardware::get_config(BLCMDConfigCommand command) {

    const leigh::jcan::Frame min_interval_request = {
            make_can_id(BLCMDSendCommand::GET_CONFIG),
            {static_cast<uint8_t>(command)},
    };
    auto start = std::chrono::steady_clock::now();
    bus_->send(min_interval_request);

    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(1)) {
        try {
            auto frame = bus_->receive_with_timeout(1000);
            if (frame.id == make_can_id(BLCMDReceiveCommand::CONFIG_DATA)) {
                auto config_value = from_bytes<T>(&frame.data[0]);
                return std::optional(config_value);
            }
        } catch (std::exception &e) {
            return std::nullopt;
        }
    }
    return std::nullopt;
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
    bus_->set_callbacks_enabled(true);
    return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn BLCMDHardware::on_deactivate(
    const rclcpp_lifecycle::State & /*previous_state*/)
{
    bus_->set_callbacks_enabled(false);
    return CallbackReturn::SUCCESS;
}

hardware_interface::return_type BLCMDHardware::read(
        const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
    bus_->spin();
    return hardware_interface::return_type::OK;
}

hardware_interface::return_type BLCMDHardware::write(
        const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
    switch(control_mode_){
        case blcmd_hardware::ControlMode::Undefined:
            break;
        case blcmd_hardware::ControlMode::Position:
            if (hw_position_.command.has_value()) {
                send_scaled<int16_t>(make_can_id(BLCMDSendCommand::DRIVE_POSITION),
                                     hw_position_.command.value(), hw_position_.max);
            } else {
                RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No position command");
                return hardware_interface::return_type::ERROR;
            }
            break;
        case blcmd_hardware::ControlMode::Velocity:
            if (hw_velocity_.command.has_value()) {
                send_scaled<int16_t>(make_can_id(BLCMDSendCommand::DRIVE_VELOCITY),
                                     hw_velocity_.command.value(), hw_velocity_.max);
            } else {
                RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No velocity command");
                return hardware_interface::return_type::ERROR;
            }
            break;
        case blcmd_hardware::ControlMode::Effort:
            if (hw_effort_.command.has_value()) {
                send_scaled<int16_t>(make_can_id(BLCMDSendCommand::DRIVE_CURRENT),
                                     hw_effort_.command.value(), hw_effort_.max);
            } else {
                RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No effort command");
                return hardware_interface::return_type::ERROR;
            }
            break;
    }
    return hardware_interface::return_type::OK;
}

hardware_interface::return_type
    BLCMDHardware::prepare_command_mode_switch(const std::vector<std::string> &start_interfaces,
                                               const std::vector<std::string> &stop_interfaces) {
    if (stop_interfaces.size() > 1){
        RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                            "Unexpected number of interfaces to stop " << stop_interfaces.size());
        return hardware_interface::return_type::ERROR;
    }

    if (start_interfaces.size() > 1){
        RCLCPP_FATAL_STREAM(rclcpp::get_logger(BLCMDHardwareLoggerName),
                            "Unexpected number of interfaces to start " << start_interfaces.size());
        return hardware_interface::return_type::ERROR;
    }

    if(stop_interfaces.size() == 1){
        if(!stop_interface(stop_interfaces[0])){
            return hardware_interface::return_type::ERROR;
        }
    }

    if (start_interfaces.size() == 1){
        if(!start_interface(start_interfaces[0])){
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

// TODO: better error handling
bool BLCMDHardware::set_control_interface(
        const hardware_interface::InterfaceInfo &interface_info, bool command) {
    if(interface_info.name == hardware_interface::HW_IF_POSITION){
        //TODO: deal with case with state interface and no command interface
        if (command){
            hw_position_.max = std::stod(interface_info.max);;
            auto resolver_reduction_search = info_.hardware_parameters.find("resolver_reduction");
            if (resolver_reduction_search == info_.joints[0].parameters.end()){
                RCLCPP_FATAL(rclcpp::get_logger(BLCMDHardwareLoggerName), "No resolver reduction provided");
                return false;
            }
            hw_position_.resolver_reduction = std::stod(resolver_reduction_search->second);
            hw_position_.command = 0.0;
        } else {
            hw_position_.state = 0.0;
        }
    } else if(interface_info.name == hardware_interface::HW_IF_VELOCITY){
        if(command) {
            hw_velocity_.max = static_cast<double>(clock_rate_)/(min_interval_*revolution_pulses_);
            hw_velocity_.command = 0.0;
        } else {
            hw_velocity_.state = 0.0;
        }
    } else if(interface_info.name == hardware_interface::HW_IF_EFFORT){
        if(command){
            hw_effort_.max = std::stod(interface_info.max);;
            hw_effort_.command = 0.0;
        } else {
            hw_effort_.state = 0.0;
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
                if (hw_velocity_.state.has_value() || hw_effort_.state.has_value()) {
            ids.push_back(make_can_id(TelemetryPacket::PACKET_1));
        }
        if (hw_position_.state.has_value()) {
            ids.push_back(make_can_id(TelemetryPacket::PACKET_3));
        }
	bus_->set_id_filter(ids);
        if (hw_velocity_.state.has_value() || hw_effort_.state.has_value())
            bus_->add_callback_to(make_can_id(TelemetryPacket::PACKET_1), this, &BLCMDHardware::packet_1_callback);
        if (hw_position_.state.has_value())
            bus_->add_callback_to(make_can_id(TelemetryPacket::PACKET_3), this, &BLCMDHardware::packet_3_callback);
        bus_->set_callbacks_enabled(false);
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

    void BLCMDHardware::packet_1_callback(leigh::jcan::Frame frame) {
    if(hw_velocity_.state.has_value()) {
        hw_velocity_.state = convert_scaled<int16_t>(&frame.data[0], hw_velocity_.max)*hw_velocity_.max;

    }
    if(hw_effort_.state.has_value()) hw_effort_.state = convert_scaled<int16_t>(&frame.data[2], hw_effort_.max);
    }

    void BLCMDHardware::packet_3_callback(leigh::jcan::Frame frame) {
        if(hw_position_.state.has_value()) hw_position_.state = convert_scaled<int16_t>(&frame.data[0], hw_position_.max)*
                hw_position_.resolver_reduction;
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
        T data = static_cast<T>( (value > max ? 1.0 : value/max)* std::numeric_limits<T>::max());
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

}  // namespace blcmd_hardware

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  blcmd_hardware::BLCMDHardware, hardware_interface::ActuatorInterface)
