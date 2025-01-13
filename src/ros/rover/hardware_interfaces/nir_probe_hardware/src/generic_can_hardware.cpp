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

#include "generic_can_hardware/generic_can_hardware.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "rclcpp/rclcpp.hpp"

#include "jcan/jcan.h"

namespace generic_can_hardware
{

hardware_interface::CallbackReturn GenericCANHardware::on_init(
        const hardware_interface::HardwareInfo & info)
{
    if (hardware_interface::SystemInterface::on_init(info) != CallbackReturn::SUCCESS)
    {
      return CallbackReturn::ERROR;
    }

    GenericCANHardwareLoggerName = info_.name;

    if (info_.gpios.size() != 1)
    {
      RCLCPP_FATAL_STREAM(
        rclcpp::get_logger(GenericCANHardwareLoggerName),
        "Hardware interface '" << info_.name << "got " << info_.gpios.size() << " gpios but expected 1");
      return CallbackReturn::ERROR;
    }

    auto canbus_search = info_.hardware_parameters.find("candevice");
    if (canbus_search == info_.hardware_parameters.end()){
        RCLCPP_FATAL(rclcpp::get_logger(GenericCANHardwareLoggerName), "No canbus provided");
        return CallbackReturn::ERROR;
    }

    can_device_ = canbus_search->second;
    RCLCPP_INFO_STREAM(rclcpp::get_logger(GenericCANHardwareLoggerName),
                "Using can device " << can_device_.c_str());

    auto canid_search = info_.hardware_parameters.find("canid");
    if (canid_search == info_.hardware_parameters.end()){
        RCLCPP_FATAL(rclcpp::get_logger(GenericCANHardwareLoggerName), "No canid provided");
        return CallbackReturn::ERROR;
    }

    can_id_ = std::stoul(canid_search->second);
    
    RCLCPP_INFO(rclcpp::get_logger(GenericCANHardwareLoggerName), "Using can id %d", can_id_);

    auto clock_rate_search = info_.hardware_parameters.find("clock_rate");
    if (clock_rate_search == info_.hardware_parameters.end()){
        RCLCPP_FATAL(rclcpp::get_logger(GenericCANHardwareLoggerName), "No clock rate provided");
        return CallbackReturn::ERROR;
    }
    clock_rate_ = std::stoul(clock_rate_search->second);
    RCLCPP_INFO_STREAM(rclcpp::get_logger(GenericCANHardwareLoggerName),
                       "Got clock rate: " << clock_rate_);

    auto nir_probe_led_enabled_search = info_.hardware_parameters.find("nir_probe_led_enabled");
    if (nir_probe_led_enabled_search != info_.hardware_parameters.end() && mock_){
        nir_probe_led_enabled_= std::stol(nir_probe_led_enabled_search->second);
    }

    auto ice_probe_led_enabled_search = info_.hardware_parameters.find("ice_probe_led_enabled");
    if (ice_probe_led_enabled_search != info_.hardware_parameters.end() && mock_){
        ice_probe_led_enabled_= std::stol(ice_probe_led_enabled_search->second);
    }

    auto water_probe_led_enabled_search = info_.hardware_parameters.find("water_probe_led_enabled");
    if (water_probe_led_enabled_search != info_.hardware_parameters.end() && mock_){
        water_probe_led_enabled_= std::stol(water_probe_led_enabled_search->second);
    }



    for (const auto& interface : info_.gpios[0].state_interfaces) {
        if (!set_control_interface(interface, false)){
            return CallbackReturn::ERROR;
        }
    }

    bus_ = leigh::jcan::new_bus();
    can_setup();

    return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn GenericCANHardware::on_configure(
        const rclcpp_lifecycle::State & previous_state)
{
  // open the can bus
    try {
        bus_->open(can_device_.c_str());
        RCLCPP_INFO(rclcpp::get_logger(GenericCANHardwareLoggerName), "Opened canbus on device %s",
                    can_device_.c_str());
    } catch (std::exception &e) {
        RCLCPP_FATAL(rclcpp::get_logger(GenericCANHardwareLoggerName), "Failed to start canbus with error: %s",
                     e.what());
        return CallbackReturn::ERROR;
    }


    if (!mock_) {
        //  get nir probe led status
        if (nir_probe_led_state_.state.has_value()) {
            RCLCPP_INFO_STREAM(rclcpp::get_logger(GenericCANHardwareLoggerName),
                               "Getting nir probe status on NIRProbe " << can_id_);
            auto nir_probe_led_status = get_config<uint16_t>(GenericCANConfigCommand::NIR_PROBE_LED_ENABLED);

            if (nir_probe_led_status.has_value()) {
                nir_probe_led_status_ = nir_probe_led_status.value();
                RCLCPP_INFO_STREAM(rclcpp::get_logger(GenericCANHardwareLoggerName),
                                   "NIR Probe LED status on " << can_id_ << " is " << nir_probe_led_status_);
            } else {
                RCLCPP_FATAL_STREAM(rclcpp::get_logger(GenericCANHardwareLoggerName),
                                    "Error getting NIR Probe LED status on " << can_id_);
                return CallbackReturn::ERROR;
            }
        }

        //  get ice probe led status
        if (ice_probe_led_state_.state.has_value()) {
            RCLCPP_INFO_STREAM(rclcpp::get_logger(GenericCANHardwareLoggerName),
                               "Getting ice probe status on NIRProbe " << can_id_);
            auto water_probe_led_status = get_config<uint16_t>(GenericCANConfigCommand::WATER_PROBE_LED_ENABLED);

            if (water_probe_led_status.has_value()) {
                water_probe_led_status_ = water_probe_led_status.value();
                RCLCPP_INFO_STREAM(rclcpp::get_logger(GenericCANHardwareLoggerName),
                                   "Water Probe LED status on " << can_id_ << " is " << water_probe_led_status_);
            } else {
                RCLCPP_FATAL_STREAM(rclcpp::get_logger(GenericCANHardwareLoggerName),
                                    "Error getting Water Probe LED status on " << can_id_);
                return CallbackReturn::ERROR;
            }
        }


        //  get water probe led status
        if (water_probe_led_state_.state.has_value()) {
            RCLCPP_INFO_STREAM(rclcpp::get_logger(GenericCANHardwareLoggerName),
                               "Getting water probe status on NIRProbe " << can_id_);
            auto water_probe_led_status = get_config<uint16_t>(GenericCANConfigCommand::WATER_PROBE_LED_ENABLED);

            if (water_probe_led_status.has_value()) {
                water_probe_led_status_ = water_probe_led_status.value();
                RCLCPP_INFO_STREAM(rclcpp::get_logger(GenericCANHardwareLoggerName),
                                   "Water Probe LED status on " << can_id_ << " is " << water_probe_led_status_);
            } else {
                RCLCPP_FATAL_STREAM(rclcpp::get_logger(GenericCANHardwareLoggerName),
                                    "Error getting Water Probe LED status on " << can_id_);
                return CallbackReturn::ERROR;
            }
        }
    }

  return CallbackReturn::SUCCESS;
}

std::vector<hardware_interface::StateInterface> GenericCANHardware::export_state_interfaces()
{
  std::vector<hardware_interface::StateInterface> state_interfaces;
  if (state_interface.state.has_value()) {
      state_interfaces.emplace_back(
              info_.gpios[0].name, state_interface.name, &state_interface.state.value());
  }

  return state_interfaces;
}

std::vector<hardware_interface::CommandInterface> GenericCANHardware::export_command_interfaces()
{
  std::vector<hardware_interface::CommandInterface> command_interfaces;

  return command_interfaces;
}

hardware_interface::CallbackReturn GenericCANHardware::on_activate(
        const rclcpp_lifecycle::State & /*previous_state*/)
{
    bus_->set_callbacks_enabled(true);
    return CallbackReturn::SUCCESS;
}

hardware_interface::CallbackReturn GenericCANHardware::on_deactivate(
    const rclcpp_lifecycle::State & /*previous_state*/)
{
    bus_->set_callbacks_enabled(false);
    return CallbackReturn::SUCCESS;
}

hardware_interface::return_type GenericCANHardware::read(
        const rclcpp::Time & time, const rclcpp::Duration & period)
{
    bus_->spin();
    return hardware_interface::return_type::OK;
}

hardware_interface::return_type GenericCANHardware::write(
        const rclcpp::Time & /*time*/, const rclcpp::Duration & /*period*/)
{
    // Write values here!
    return hardware_interface::return_type::OK;
}

hardware_interface::return_type
    GenericCANHardware::prepare_command_mode_switch(const std::vector<std::string> &start_interfaces,
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

bool GenericCANHardware::stop_interface(const std::string &interface) {
    size_t delimiter_pos = interface.find('/');

    std::string gpio_name = interface.substr(0, delimiter_pos);
    std::string interface_name = interface.substr(delimiter_pos + 1);

    if (gpio_name != info_.gpios[0].name){
        return true;
    }

    state_interface.state = 0;
    state_interface.name = interface_name;

    RCLCPP_FATAL_STREAM(rclcpp::get_logger(GenericCANHardwareLoggerName),
                        "Unexpected interface " << interface_name.c_str());
    return false;
}

bool GenericCANHardware::start_interface(const std::string &interface) {
    size_t delimiter_pos = interface.find('/');

    std::string joint_name = interface.substr(0, delimiter_pos);
    std::string interface_name = interface.substr(delimiter_pos + 1);

    return true;
}

// TODO: better error handling
bool GenericCANHardware::set_control_interface(
        const hardware_interface::InterfaceInfo &interface_info, bool command) {

    if (command)
        return false;

    state_interface.state = 0;
    state_interface.max = std::stod(interface_info.max);
    return true;
}

    void GenericCANHardware::can_setup() {

        std::vector<uint32_t> ids = {
                can_id_
        };

        bus_->set_id_filter(ids);

        if (state_interface.state.has_value()) {
            RCLCPP_INFO_STREAM(rclcpp::get_logger(GenericCANHardwareLoggerName),
                               "Adding packet callback to ID: " << can_id_);
            bus_->add_callback_to(static_cast<int>(can_id_), this, &GenericCANHardware::packet_callback);
        }

        // Later enabled in on_activate
        bus_->set_callbacks_enabled(false);
   }

    void GenericCANHardware::packet_callback(leigh::jcan::Frame frame) {
        if(state_interface.state.has_value()) {
            state_interface.state = convert_scaled<int16_t>(&frame.data[0], state_interface.max);
        }
    }

    template<typename T>
    double GenericCANHardware::convert_scaled(const uint8_t *bytes, double max) {
        return static_cast<double>(from_bytes<T>(bytes))/ std::numeric_limits<T>::max() * max;
    }

    template<typename T>
    T GenericCANHardware::from_bytes(const uint8_t *bytes) {
        T data = bytes[0];
        for(unsigned int i = 1; i < sizeof(T); i++) {
            data = data << 8 | bytes[i];
        }
        return data;
    }

    template<typename T>
    void GenericCANHardware::send_scaled(uint32_t id, double value, double max) {
        T data = static_cast<T>( (abs(value) > max ? (value > 0 ? 1 : -1) : value/max)* std::numeric_limits<T>::max());
        send_raw(id, data);
    }

    template<typename T>
    void GenericCANHardware::send_raw(const uint32_t id, T data) {
        leigh::jcan::Frame frame;
        frame.id = id;
        for(unsigned int i = 0; i < sizeof(T); i++) {
            frame.data.push_back(data >> 8*(sizeof(T) - (i + 1)) & 0xFF);
        }
        bus_->send(frame);
    }

}  // namespace generic_can_hardware

#include "pluginlib/class_list_macros.hpp"

PLUGINLIB_EXPORT_CLASS(
  generic_can_hardware::GenericCANHardware, hardware_interface::SystemInterface)
