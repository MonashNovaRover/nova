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

#include "cmd_hardware/cmd_hardware.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

#include "jcan/jcan.h"

namespace cmd_hardware
{
hardware_interface::CallbackReturn CMDHardware::on_init(
        const hardware_interface::HardwareInfo & info)
{
    if (hardware_interface::SystemInterface::on_init(info) != CallbackReturn::SUCCESS)
    {
      return CallbackReturn::ERROR;
    }

    CMDHardwareLoggerName = info_.name;

    auto params_result = apply_parameters();
    if (params_result != CallbackReturn::SUCCESS)
      return params_result;

    if (info_.joints.size() != 1)
    {
        RCLCPP_FATAL_STREAM(
          rclcpp::get_logger(CMDHardwareLoggerName),
          "Hardware interface '" << info_.name << "got " << info_.joints.size() << " joints but expected 1");
        return CallbackReturn::ERROR;
    }

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

    control_mode_ = cmd_hardware::ControlMode::Undefined;
	
    bus_ = leigh::jcan::new_bus();
    can_setup();

    return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn CMDHardware::on_configure(
        const rclcpp_lifecycle::State & previous_state)
{
    // open the can bus
    try {
        bus_->open(params_.candevice.c_str());
        RCLCPP_INFO(rclcpp::get_logger(CMDHardwareLoggerName), "Opened canbus on device %s",
                    params_.candevice.c_str());
    } catch (std::exception &e) {
        RCLCPP_FATAL(rclcpp::get_logger(CMDHardwareLoggerName), "Failed to start canbus with error: %s",
                     e.what());
        return CallbackReturn::ERROR;
    }

    // check for resolver if there is a position interface
    if (hw_position_.state.has_value() || hw_position_.command.has_value()) {
//        RCLCPP_INFO_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
//                           "Checking for resolver on CMD " << can_id_);

//        auto resolver_check = get_config<uint16_t>(CMDConfigCommand::HAS_RESOLVER);
//        if (resolver_check.has_value()) {
//            if (resolver_check.value()) {
//                RCLCPP_INFO_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
//                                   "Resolver detected on CMD " << can_id_);
//                return CallbackReturn::SUCCESS;
//            } else {
//                RCLCPP_FATAL_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
//                                    "No resolver detected on CMD " << can_id_);
//                return CallbackReturn::ERROR;
//            }
//        }
//        RCLCPP_FATAL_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
//                            "Error with resolver request on CMD" << can_id_);
//        return CallbackReturn::ERROR;
    }

    return CallbackReturn::SUCCESS;
}

template<typename T>
std::optional<T> CMDHardware::get_config(CMDConfigCommand command) {

//    const leigh::jcan::Frame min_interval_request = {
//            make_can_id(CMDSendCommand::GET_CONFIG),
//            {static_cast<uint8_t>(command)},
//    };
//    auto start = std::chrono::steady_clock::now();
//    bus_->send(min_interval_request);
//
//    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(1)) {
//        try {
//            auto frame = bus_->receive_with_timeout(1000);
//            if (frame.id == make_can_id(CMDReceiveCommand::CONFIG_DATA)) {
//                auto config_value = from_bytes<T>(&frame.data[0]);
//                return std::optional(config_value);
//            }
//        } catch (std::exception &e) {
//            return std::nullopt;
//        }
//    }
    return std::nullopt;
}

std::vector<hardware_interface::StateInterface> CMDHardware::export_state_interfaces()
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

std::vector<hardware_interface::CommandInterface> CMDHardware::export_command_interfaces()
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

hardware_interface::CallbackReturn CMDHardware::on_activate(
        const rclcpp_lifecycle::State & /*previous_state*/)
{
    bus_->set_callbacks_enabled(true);
    return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn CMDHardware::on_deactivate(
    const rclcpp_lifecycle::State & /*previous_state*/)
{
    bus_->set_callbacks_enabled(false);
    return CallbackReturn::SUCCESS;
}

hardware_interface::return_type CMDHardware::read(
        const rclcpp::Time & time, const rclcpp::Duration & period)
{
    bus_->spin();

    const auto previous_position_state = hw_position_.state.value();

    // Transfer reference states to the actual state interfaces

    // Apply interpolation between CAN feedback and previous command to velocity
    //hw_velocity_.state = lerp(hw_velocity_.reference_state, hw_velocity_.reference_command,
    //                          params_.velocity_integration_command_amount);

    // Calculate position from resolver values
    const auto reference_resolver_state = hw_position_.raw_reference_state + 2*M_PI * hw_position_.raw_reference_state_turns;
    // Apply resolver reduction
    const auto reference_position_state = reference_resolver_state / params_.resolver_reduction;
    // Apply velocity integration to position
    hw_position_.state = reference_position_state;
    //    + hw_velocity_.state.value() * params_.velocity_integration_seconds;

    // Infer velocity state from the resolver, since we don't trust encoder feedback
    hw_velocity_.state = (reference_position_state - previous_position_state) / period.seconds();

    return hardware_interface::return_type::OK;
}

hardware_interface::return_type CMDHardware::write(
        const rclcpp::Time & /*time*/, const rclcpp::Duration & period)
{
    switch (control_mode_) {
        case cmd_hardware::ControlMode::Undefined:
            break;
        case cmd_hardware::ControlMode::Position:
            if (hw_position_.command.has_value()) {
                RCLCPP_DEBUG_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
                                   "Sending Position Command " << hw_position_.command.value());

                const auto position_cmd = hw_position_.command.value();
                const auto position_state = hw_position_.state.value();

                // Error term to do velocity seeking on
                const auto position_error = position_cmd - position_state;

                hw_velocity_.reference_command = params_.position_seeking_velocity_multiplier * position_error / period.seconds();
            }
            else {
                RCLCPP_FATAL(rclcpp::get_logger(CMDHardwareLoggerName), "No position command");
                return hardware_interface::return_type::ERROR;
            }
            break;

        case cmd_hardware::ControlMode::Velocity:
            if (hw_velocity_.command.has_value()) {
                hw_velocity_.reference_command = hw_velocity_.command.value();
            }
            else {
                RCLCPP_FATAL(rclcpp::get_logger(CMDHardwareLoggerName), "No velocity command");
                return hardware_interface::return_type::ERROR;
            }
            break;

        case cmd_hardware::ControlMode::Effort:
            if (hw_effort_.command.has_value()) {
                send_scaled<int16_t>(make_can_id(CMDSendCommand::PWM_DRIVE),
                                     hw_effort_.command.value() * reverse_velocity_multiplier_, params_.max_effort);
            }
            else {
                RCLCPP_FATAL(rclcpp::get_logger(CMDHardwareLoggerName), "No effort command");
                return hardware_interface::return_type::ERROR;
            }
            break;
    }

    // Actually send velocity commands
    if (control_mode_ == ControlMode::Position || control_mode_ == ControlMode::Velocity) {
        RCLCPP_DEBUG_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
                          "Sending velocity command " << hw_velocity_.reference_command * reverse_velocity_multiplier_);
        send_scaled<int16_t>(make_can_id(CMDSendCommand::PID_DRIVE),
                             hw_velocity_.reference_command * reverse_velocity_multiplier_, params_.max_velocity);
    }

    return hardware_interface::return_type::OK;
}

hardware_interface::return_type
    CMDHardware::prepare_command_mode_switch(const std::vector<std::string> &start_interfaces,
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

bool CMDHardware::stop_interface(const std::string &interface){

    size_t delimiter_pos = interface.find('/');

    std::string joint_name = interface.substr(0, delimiter_pos);
    std::string interface_name = interface.substr(delimiter_pos + 1);

    if (joint_name != info_.joints[0].name){
        return true;
    }

    if (interface_name == hardware_interface::HW_IF_POSITION) {
        if (control_mode_ == cmd_hardware::ControlMode::Position) {
            control_mode_ = cmd_hardware::ControlMode::Undefined;
            return true;
        } else {
            RCLCPP_FATAL_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
                                "Requested stop position when control mode was not position");
            return false;
        }
    }
    if (interface_name == hardware_interface::HW_IF_VELOCITY) {
        if (control_mode_ == cmd_hardware::ControlMode::Velocity) {
            hw_velocity_.command = 0.0;
            control_mode_ = cmd_hardware::ControlMode::Undefined;
            return true;
        } else {
            RCLCPP_FATAL_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
                                "Requested stop " << interface_name.c_str() << "when control mode was not velocity");
            return false;
        }
    }
    if (interface_name == hardware_interface::HW_IF_EFFORT) {
        if (control_mode_ == cmd_hardware::ControlMode::Effort) {
            hw_effort_.command = 0.0;
            control_mode_ = cmd_hardware::ControlMode::Undefined;
            return true;
        } else {
            RCLCPP_FATAL_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
                                "Requested stop " << interface_name.c_str() << "when control mode was not effort");
            return false;
        }
    }

    RCLCPP_FATAL_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
                        "Unexpected interface " << interface_name.c_str());
    return false;

}

bool CMDHardware::start_interface(const std::string &interface){

    size_t delimiter_pos = interface.find('/');

    std::string joint_name = interface.substr(0, delimiter_pos);
    std::string interface_name = interface.substr(delimiter_pos + 1);

    if (joint_name != info_.joints[0].name){
        return true;
    }

    if (control_mode_ != cmd_hardware::ControlMode::Undefined) {
        RCLCPP_FATAL_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
                            "Requested start interface " << interface.c_str() << " when control mode was not undefined");
        return false;
    }

    if (interface_name == hardware_interface::HW_IF_POSITION){
        control_mode_ = cmd_hardware::ControlMode::Position;
    } else if (interface_name == hardware_interface::HW_IF_VELOCITY){
        control_mode_ = cmd_hardware::ControlMode::Velocity;
    } else if (interface_name == hardware_interface::HW_IF_EFFORT){
        control_mode_ = cmd_hardware::ControlMode::Effort;
    } else {
        RCLCPP_FATAL_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
                            "Unexpected interface " << interface_name.c_str());
        return false;
    }

    return true;
}

hardware_interface::CallbackReturn CMDHardware::apply_parameters() {
    auto canbus_search = info_.hardware_parameters.find("candevice");
    if (canbus_search == info_.hardware_parameters.end()){
      RCLCPP_FATAL(rclcpp::get_logger(CMDHardwareLoggerName), "No canbus provided");
      return CallbackReturn::ERROR;
    }
    params_.candevice = canbus_search->second;
    RCLCPP_INFO_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
                       "Using can device " << params_.candevice.c_str());

    auto canid_search = info_.hardware_parameters.find("canid");
    if (canid_search == info_.hardware_parameters.end()){
      RCLCPP_FATAL(rclcpp::get_logger(CMDHardwareLoggerName), "No canid provided");
      return CallbackReturn::ERROR;
    }
    params_.canid = std::stoul(canid_search->second);
    RCLCPP_INFO(rclcpp::get_logger(CMDHardwareLoggerName), "Using can id %d", params_.canid);

    auto resolver_id_search = info_.hardware_parameters.find("resolver_id");
    if (resolver_id_search == info_.hardware_parameters.end()){
      RCLCPP_FATAL(rclcpp::get_logger(CMDHardwareLoggerName), "No resolver_id provided");
      return CallbackReturn::ERROR;
    }
    params_.resolver_id = static_cast<uint8_t>(std::stoul(resolver_id_search->second));
    RCLCPP_INFO(rclcpp::get_logger(CMDHardwareLoggerName), "Using resolver id %d", params_.resolver_id);

    auto mock_search = info_.hardware_parameters.find("mock");
    if (mock_search != info_.hardware_parameters.end()) {
      params_.mock = is_true(mock_search->second);
    }

    auto reverse_velocity_search = info_.hardware_parameters.find("reverse_velocity");
    if (reverse_velocity_search != info_.hardware_parameters.end() && is_true(reverse_velocity_search->second)) {
      RCLCPP_INFO_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
                         "Interface is reverse_velocity");
      params_.reverse_velocity = true;
      reverse_velocity_multiplier_ = -1;
    }

    auto reverse_velocity_feedback_search = info_.hardware_parameters.find("reverse_velocity_feedback");
    if (reverse_velocity_feedback_search != info_.hardware_parameters.end() && is_true(reverse_velocity_feedback_search->second)) {
      RCLCPP_INFO_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
                         "Interface is reverse_velocity_feedback");
      params_.reverse_velocity_feedback = true;
      reverse_velocity_feedback_multiplier_ = -reverse_velocity_multiplier_;
    }

    auto reverse_position_search = info_.hardware_parameters.find("reverse_position");
    if (reverse_position_search != info_.hardware_parameters.end() && is_true(reverse_position_search->second)) {
      RCLCPP_INFO_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
                         "Interface is reverse_position");
      params_.reverse_position = true;
      reverse_position_multiplier_ = -1;
    }

    auto max_position_search = info_.hardware_parameters.find("max_position");
    if (max_position_search != info_.hardware_parameters.end()) {
      params_.max_position = std::stod(max_position_search->second);
      RCLCPP_INFO(rclcpp::get_logger(CMDHardwareLoggerName), "Using max_position of %f", params_.max_position);
    }
    else {
      RCLCPP_INFO(rclcpp::get_logger(CMDHardwareLoggerName), "max_position parameter was undefined. Will use "
                                                               "max_position from command interface.");
    }

    auto max_velocity_search = info_.hardware_parameters.find("max_velocity");
    if (max_velocity_search != info_.hardware_parameters.end()) {
      params_.max_velocity = std::stod(max_velocity_search->second);
      RCLCPP_INFO(rclcpp::get_logger(CMDHardwareLoggerName), "Using max_velocity of %f", params_.max_velocity);
    }
    else {
      RCLCPP_INFO(rclcpp::get_logger(CMDHardwareLoggerName), "max_velocity parameter was undefined.");
    }

    auto max_effort_search = info_.hardware_parameters.find("max_effort");
    if (max_effort_search != info_.hardware_parameters.end()) {
      params_.max_effort = std::stod(max_effort_search->second);
      RCLCPP_INFO(rclcpp::get_logger(CMDHardwareLoggerName), "Using max_effort of %f", params_.max_effort);
    }
    else {
      RCLCPP_INFO(rclcpp::get_logger(CMDHardwareLoggerName), "max_effort parameter was undefined.");
    }

    auto resolver_reduction_search = info_.hardware_parameters.find("resolver_reduction");
    if (resolver_reduction_search != info_.hardware_parameters.end()) {
      params_.resolver_reduction = std::stod(resolver_reduction_search->second);
    }

    auto velocity_integration_seconds_search = info_.hardware_parameters.find("velocity_integration_seconds");
    if (velocity_integration_seconds_search != info_.hardware_parameters.end()) {
      params_.velocity_integration_seconds = std::stod(velocity_integration_seconds_search->second);
    }

    auto velocity_integration_command_amount_search = info_.hardware_parameters.find("velocity_integration_command_amount");
    if (velocity_integration_command_amount_search != info_.hardware_parameters.end()) {
      params_.velocity_integration_command_amount = std::stod(velocity_integration_command_amount_search->second);
    }

    auto position_seeking_velocity_multiplier_search = info_.hardware_parameters.find("position_seeking_velocity_multiplier");
    if (position_seeking_velocity_multiplier_search != info_.hardware_parameters.end()) {
      params_.position_seeking_velocity_multiplier = std::stod(position_seeking_velocity_multiplier_search->second);
    }

    auto position_offset_search = info_.hardware_parameters.find("position_offset");
    if (position_offset_search != info_.hardware_parameters.end()) {
      params_.position_offset = std::stod(position_offset_search->second);

      RCLCPP_INFO(rclcpp::get_logger(CMDHardwareLoggerName), "Using position_offest of: %f", params_.position_offset);
    }

    return CallbackReturn::SUCCESS;
}

// TODO: better error handling
bool CMDHardware::set_control_interface(
        const hardware_interface::InterfaceInfo &interface_info, bool command) {
    if (interface_info.name == hardware_interface::HW_IF_POSITION){
        //TODO: deal with case with state interface and no command interface
        if (command){
//            hw_position_.max = std::stod(interface_info.max);
            // auto resolver_reduction_search = info_.hardware_parameters.find("resolver_reduction");
            // if (resolver_reduction_search == info_.joints[0].parameters.end()){
            //     RCLCPP_FATAL(rclcpp::get_logger(CMDHardwareLoggerName), "No resolver reduction provided");
            //     return false;
            // }
            // hw_position_.resolver_reduction = std::stod(resolver_reduction_search->second);
            hw_position_.command = 0.0;
            hw_velocity_.command = 0.0;
            hw_velocity_.state = 0.0;
            hw_velocity_.reference_command = 0.0;
            hw_position_.raw_reference_state = 0.0;
            // Don't reset raw_reference_state_turns, so it persists between uses of the hw interface
            // hw_position_.raw_reference_state_valid = false;
        }
        else {
            hw_position_.state = 0.0;
            hw_velocity_.command = 0.0;
            hw_velocity_.state = 0.0;
        }
    } else if (interface_info.name == hardware_interface::HW_IF_VELOCITY){
        if (command) {
            RCLCPP_INFO_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
            "Configured velocity interface with max velocity: " << params_.max_velocity);
            hw_velocity_.command = 0.0;
            hw_velocity_.reference_command = 0.0;
        }
        else {
            hw_velocity_.state = 0.0;
        }
    } else if (interface_info.name == hardware_interface::HW_IF_EFFORT){
        if (command){
//            hw_effort_.max = std::stod(interface_info.max);;
            hw_effort_.command = 0.0;
        }
        else {
            hw_effort_.state = 0.0;
        }
    } else {
        RCLCPP_FATAL(rclcpp::get_logger(CMDHardwareLoggerName), "Unexpected interface %s",
                     interface_info.name.c_str());
        return false;
    }
    return true;
}

    void CMDHardware::can_setup() {
        std::vector<uint32_t> ids = {};

        if (hw_velocity_.state.has_value() || hw_effort_.state.has_value()) {
//            ids.push_back(make_can_id(TelemetryPacket::PACKET_1));
        }
        if (hw_position_.state.has_value() || hw_position_.command.has_value()) {
            ids.push_back(static_cast<uint32_t>(TelemetryPacket::RESOLVER_ARBITRATION_ID));
        }

        bus_->set_id_filter(ids);

        if (hw_velocity_.state.has_value()) {
//            RCLCPP_INFO_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
//                               "Adding packet 1 callback to ID:" << make_can_id(TelemetryPacket::PACKET_1));
//            bus_->add_callback_to(make_can_id(TelemetryPacket::PACKET_1), this, &CMDHardware::packet_1_callback);
        }
        if (hw_position_.state.has_value()) {
            RCLCPP_INFO_STREAM(rclcpp::get_logger(CMDHardwareLoggerName),
                               "Adding resolver callback to the RESOLVER_ARBITRATION_ID");
            bus_->add_callback_to(static_cast<uint32_t>(TelemetryPacket::RESOLVER_ARBITRATION_ID), this, &CMDHardware::resolver_callback);
        }
        bus_->set_callbacks_enabled(false);
    }

    uint32_t CMDHardware::make_can_id(CMDSendCommand command) const
    {
        return static_cast<uint32_t>(CanIdPrefix::SEND) << 8 | params_.canid << 4 |
               static_cast<uint32_t>(command);
    }

    uint32_t CMDHardware::make_can_id(CMDReceiveCommand command) const
    {
        return static_cast<uint32_t>(CanIdPrefix::RECEIVE) << 8 | params_.canid << 4 |
               static_cast<uint32_t>(command);
    }

    uint32_t CMDHardware::make_can_id(TelemetryPacket packet) const
    {
        return static_cast<uint32_t>(CanIdPrefix::RECEIVE) << 8 | params_.canid << 4 |
               static_cast<uint32_t>(packet);
    }

    void CMDHardware::resolver_callback(leigh::jcan::Frame frame) {
        if (!hw_position_.state.has_value())
            return;

        // Filter out messages from resolvers for other joints
        uint8_t resolver_id = frame.data[0];
        if (resolver_id != params_.resolver_id)
            return;

        // Check flags byte to make sure the message is valid
        uint8_t flags = frame.data[1];
        if (flags) {
            if (flags & static_cast<uint8_t>(ResolverFlags::RS485_READ_TIMEOUT)) {
                RCLCPP_WARN(rclcpp::get_logger(CMDHardwareLoggerName), "CMD Resolver RS485 read timout for %d",
                            params_.resolver_id);
            }
            if (flags & static_cast<uint8_t>(ResolverFlags::INVALID_CHECKSUM)) {
                RCLCPP_WARN(rclcpp::get_logger(CMDHardwareLoggerName), "CMD Resolver sent an invalid checksum for %d",
                            params_.resolver_id);
            }
            return;
        }

        // Unpack the resolver value, convert 14 bit value to signed 16.
        // const uint16_t raw_value = (frame.data[2] << 10) | (frame.data[3] << 2)
        //   * reverse_position_multiplier_;
        // const int16_t value = static_cast<int16_t>(raw_value);

        const auto value = static_cast<int16_t>((static_cast<uint16_t>(frame.data[2]) << 10) | static_cast<uint16_t>(frame.data[3] << 2))
          * reverse_position_multiplier_;

        RCLCPP_INFO(rclcpp::get_logger(CMDHardwareLoggerName), "Raw resolver: %d", value);

        if (!hw_position_.raw_reference_state_valid) {
          // Prevent phantom turn count increments on interface initialisation
          hw_position_.raw_reference_state = raw_resolver_to_rad(value);
          hw_position_.raw_reference_state_valid = true;
          return;
        }

        const auto last_raw_ref = hw_position_.raw_reference_state;
        hw_position_.raw_reference_state = raw_resolver_to_rad(value);

        const auto raw_delta = hw_position_.raw_reference_state - last_raw_ref;

        // Modifying raw_reference_state_turns effectively adds or subtracts 2*M_PI, emulating multi-turn
        if (raw_delta < -M_PI) {
            hw_position_.raw_reference_state_turns++;
            RCLCPP_INFO(rclcpp::get_logger(CMDHardwareLoggerName), "Incremented to %d turns!", hw_position_.raw_reference_state_turns);
        }
        else if (raw_delta > M_PI) {
            hw_position_.raw_reference_state_turns--;
            RCLCPP_INFO(rclcpp::get_logger(CMDHardwareLoggerName), "Decremented to %d turns!", hw_position_.raw_reference_state_turns);
        }
        else if (abs(raw_delta) == M_PI) {
            // This is the edge case. It is ambiguous which direction has been turned! So, guess from the velocity
            const auto velocity = hw_velocity_.state.has_value() ? hw_velocity_.state.value() : hw_velocity_.reference_command;
            if (velocity >= 0) 
              hw_position_.raw_reference_state_turns++;
            else 
              hw_position_.raw_reference_state_turns--;
        }
    }

    template<typename T>
    double CMDHardware::convert_scaled(const uint8_t *bytes, double max) {
        return static_cast<double>(from_bytes<T>(bytes))/ std::numeric_limits<T>::max() * max;
    }

    template<typename T>
    T CMDHardware::from_bytes(const uint8_t *bytes) {
        T data = bytes[0];
        for(unsigned int i = 1; i < sizeof(T); i++) {
            data = data << 8 | bytes[i];
        }
        return data;
    }

    template<typename T>
    void CMDHardware::send_scaled(uint32_t id, double value, double max) {
        T data = static_cast<T>( (abs(value) > max ? (value > 0 ? 1 : -1) : value/max)* std::numeric_limits<T>::max());
        send_raw(id, data);
    }

    template<typename T>
    void CMDHardware::send_raw(const uint32_t id, T data) {
        leigh::jcan::Frame frame;
        frame.id = id;
        for(unsigned int i = 0; i < sizeof(T); i++) {
            frame.data.push_back(data >> 8*(sizeof(T) - (i + 1)) & 0xFF);
        }
        bus_->send(frame);
    }

    bool CMDHardware::is_true(std::string& text) {
        return text == "true" || text == "True";
    }

    double CMDHardware::raw_resolver_to_rad(int16_t raw_resolver_data) {
        return M_PI * static_cast<double>(raw_resolver_data) / 0x7FFF;
    }

    inline double CMDHardware::lerp(const double a, const double b, const double t) {
        return (1-t)*a + t*b;
    }

}  // namespace cmd_hardware

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  cmd_hardware::CMDHardware, hardware_interface::SystemInterface)
