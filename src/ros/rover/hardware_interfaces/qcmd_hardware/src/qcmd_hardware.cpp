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

#include "qcmd_hardware/qcmd_hardware.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

#include "jcan/jcan.h"

namespace qcmd_hardware
{
hardware_interface::CallbackReturn QCMDHardware::on_init(
        const hardware_interface::HardwareInfo & info)
{
    if (hardware_interface::SystemInterface::on_init(info) != CallbackReturn::SUCCESS)
    {
      return CallbackReturn::ERROR;
    }

    QCMDHardwareLoggerName = info_.name;

    auto params_result = apply_parameters();
    if (params_result != CallbackReturn::SUCCESS)
      return params_result;

    if (info_.joints.size() != 1)
    {
        RCLCPP_FATAL_STREAM(
          rclcpp::get_logger(QCMDHardwareLoggerName),
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
	
    bus_ = leigh::jcan::new_bus();
    can_setup();

    return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn QCMDHardware::on_configure(
        const rclcpp_lifecycle::State & previous_state)
{
    // open the can bus
    try {
        bus_->open(params_.candevice.c_str());
        RCLCPP_INFO(rclcpp::get_logger(QCMDHardwareLoggerName), "Opened canbus on device %s",
                    params_.candevice.c_str());
    } catch (std::exception &e) {
        RCLCPP_FATAL(rclcpp::get_logger(QCMDHardwareLoggerName), "Failed to start canbus with error: %s",
                     e.what());
        return CallbackReturn::ERROR;
    }

    return CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> QCMDHardware::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  if (hw_effort_.state.has_value()) {
      state_interfaces.emplace_back(
              info_.joints[0].name, hardware_interface::HW_IF_VELOCITY, &hw_effort_.state.value());
  }

  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> QCMDHardware::export_command_interfaces()
{
    std::vector<hardware_interface::CommandInterface> command_interfaces;

    if (hw_effort_.command.has_value()) {
        command_interfaces.emplace_back(
                info_.joints[0].name, hardware_interface::HW_IF_VELOCITY, &hw_effort_.command.value());
    }

    return command_interfaces;
}

hardware_interface::CallbackReturn QCMDHardware::on_activate(
        const rclcpp_lifecycle::State & /*previous_state*/)
{
    bus_->set_callbacks_enabled(true);
    return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn QCMDHardware::on_deactivate(
    const rclcpp_lifecycle::State & /*previous_state*/)
{
    bus_->set_callbacks_enabled(false);
    return CallbackReturn::SUCCESS;
}

hardware_interface::return_type QCMDHardware::read(
        const rclcpp::Time & time, const rclcpp::Duration & period)
{
    bus_->spin();

    return hardware_interface::return_type::OK;
}

hardware_interface::return_type QCMDHardware::write(
        const rclcpp::Time & /*time*/, const rclcpp::Duration & period)
{
    if (hw_effort_.command.has_value()) {
        hw_effort_.reference_command = hw_effort_.command.value();
    }
    else {
        RCLCPP_FATAL(rclcpp::get_logger(QCMDHardwareLoggerName), "No effort command");
        return hardware_interface::return_type::ERROR;
    }

    send_scaled<int16_t>(params_.canid,
                        hw_effort_.reference_command, params_.max_effort);

    return hardware_interface::return_type::OK;
}


bool QCMDHardware::stop_interface(const std::string &interface){

    size_t delimiter_pos = interface.find('/');

    std::string joint_name = interface.substr(0, delimiter_pos);
    std::string interface_name = interface.substr(delimiter_pos + 1);

    if (joint_name != info_.joints[0].name){
        return true;
    }

    if (interface_name == hardware_interface::HW_IF_VELOCITY) {
        hw_effort_.command = 0.0;
        return true;
    }

    RCLCPP_FATAL_STREAM(rclcpp::get_logger(QCMDHardwareLoggerName),
                        "Unexpected interface " << interface_name.c_str());
    return false;

}

bool QCMDHardware::start_interface(const std::string &interface){

    size_t delimiter_pos = interface.find('/');

    std::string joint_name = interface.substr(0, delimiter_pos);
    std::string interface_name = interface.substr(delimiter_pos + 1);

    if (joint_name != info_.joints[0].name){
        return true;
    }

    return true;
}

hardware_interface::CallbackReturn QCMDHardware::apply_parameters() {
    auto canbus_search = info_.hardware_parameters.find("candevice");
    if (canbus_search == info_.hardware_parameters.end()){
      RCLCPP_FATAL(rclcpp::get_logger(QCMDHardwareLoggerName), "No canbus provided");
      return CallbackReturn::ERROR;
    }
    params_.candevice = canbus_search->second;
    RCLCPP_INFO_STREAM(rclcpp::get_logger(QCMDHardwareLoggerName),
                       "Using can device " << params_.candevice.c_str());

    auto canid_search = info_.hardware_parameters.find("canid");
    if (canid_search == info_.hardware_parameters.end()){
      RCLCPP_FATAL(rclcpp::get_logger(QCMDHardwareLoggerName), "No canid provided");
      return CallbackReturn::ERROR;
    }
    params_.canid = std::stoul(canid_search->second, nullptr, 16);
    RCLCPP_INFO(rclcpp::get_logger(QCMDHardwareLoggerName), "Using can id %d", params_.canid);

    auto mock_search = info_.hardware_parameters.find("mock");
    if (mock_search != info_.hardware_parameters.end()) {
      params_.mock = is_true(mock_search->second);
    }

    auto max_effort_search = info_.hardware_parameters.find("max_effort");
    if (max_effort_search != info_.hardware_parameters.end()) {
      params_.max_effort = std::stod(max_effort_search->second);
      RCLCPP_INFO(rclcpp::get_logger(QCMDHardwareLoggerName), "Using max_effort of %f", params_.max_effort);
    }
    else {
      RCLCPP_INFO(rclcpp::get_logger(QCMDHardwareLoggerName), "max_effort parameter was undefined.");
    }

    return CallbackReturn::SUCCESS;
}

hardware_interface::return_type
    QCMDHardware::prepare_command_mode_switch(const std::vector<std::string> &start_interfaces,
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

// TODO: better error handling
bool QCMDHardware::set_control_interface(
        const hardware_interface::InterfaceInfo &interface_info, bool command) {
   if (interface_info.name == hardware_interface::HW_IF_VELOCITY){
        if (command){
//            hw_effort_.max = std::stod(interface_info.max);;
            hw_effort_.command = 0.0;
        }
        else {
            hw_effort_.state = 0.0;
        }
    } else {
        RCLCPP_FATAL(rclcpp::get_logger(QCMDHardwareLoggerName), "Unexpected interface %s",
                     interface_info.name.c_str());
        return false;
    }
    return true;
}

    void QCMDHardware::can_setup() {
        std::vector<uint32_t> ids = {};

//         if (hw_velocity_.state.has_value() || hw_effort_.state.has_value()) {
// //            ids.push_back(make_can_id(TelemetryPacket::PACKET_1));
//         }

        bus_->set_id_filter(ids);

        bus_->set_callbacks_enabled(false);
    }


    template<typename T>
    double QCMDHardware::convert_scaled(const uint8_t *bytes, double max) {
        return static_cast<double>(from_bytes<T>(bytes))/ std::numeric_limits<T>::max() * max;
    }

    template<typename T>
    T QCMDHardware::from_bytes(const uint8_t *bytes) {
        T data = bytes[0];
        for(unsigned int i = 1; i < sizeof(T); i++) {
            data = data << 8 | bytes[i];
        }
        return data;
    }

    template<typename T>
    void QCMDHardware::send_scaled(uint32_t id, double value, double max) {
        T data = static_cast<T>( (abs(value) > max ? (value > 0 ? 1 : -1) : value/max)* std::numeric_limits<T>::max());
        send_raw(id, data);
    }

    template<typename T>
    void QCMDHardware::send_raw(const uint32_t id, T data) {
        leigh::jcan::Frame frame;
        frame.id = id;
        for(unsigned int i = 0; i < sizeof(T); i++) {
            frame.data.push_back(data >> 8*(sizeof(T) - (i + 1)) & 0xFF);
        }
        bus_->send(frame);
    }

    bool QCMDHardware::is_true(std::string& text) {
        return text == "true" || text == "True";
    }

}  // namespace qcmd_hardware

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  qcmd_hardware::QCMDHardware, hardware_interface::SystemInterface)
