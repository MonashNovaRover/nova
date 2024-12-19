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

#ifndef AUGER_HARDWARE__AUGER_HARDWARE_HPP_
#define AUGER_HARDWARE__AUGER_HARDWARE_HPP_

#include <string>
#include <vector>
#include <optional>

#include "rclcpp/rclcpp.hpp"
#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp/macros.hpp"
#include "rclcpp_lifecycle/state.hpp"

#include "jcan/jcan.h"

namespace auger_hardware
{

struct ControlInterface {
    std::optional<double> command;
    std::optional<double> state;
    double max {std::numeric_limits<double>::quiet_NaN()};
};

enum class ControlMode {
    Undefined,
    Position,
    Velocity,
    Effort,
};

class AugerHardware : public hardware_interface::SystemInterface
{
public:
    hardware_interface::CallbackReturn on_init(
        const hardware_interface::HardwareInfo & info) override;

    hardware_interface::CallbackReturn on_configure(
        const rclcpp_lifecycle::State & previous_state) override;

    std::vector<hardware_interface::StateInterface> export_state_interfaces() override;

    std::vector<hardware_interface::CommandInterface> export_command_interfaces() override;

    hardware_interface::CallbackReturn on_activate(
        const rclcpp_lifecycle::State & previous_state) override;

    hardware_interface::CallbackReturn on_deactivate(
        const rclcpp_lifecycle::State & previous_state) override;

    hardware_interface::return_type read(
        const rclcpp::Time & time, const rclcpp::Duration & period) override;

    hardware_interface::return_type write(
        const rclcpp::Time & time, const rclcpp::Duration & period) override;

    hardware_interface::return_type prepare_command_mode_switch(
        const std::vector<std::string> & start_interfaces,
        const std::vector<std::string> & stop_interfaces) override;

private:
    std::string AugerHardwareLoggerName;

    ControlInterface hw_effort_;

    ControlMode control_mode_;

    std::unique_ptr<leigh::jcan::Bus> bus_;
    std::basic_string<char> can_device_;

    uint32_t can_id_;

    uint32_t clock_rate_;


    bool set_control_interface(const hardware_interface::InterfaceInfo & interface_info, bool command);

    bool stop_interface(const std::string &interface);

    bool start_interface(const std::string &interface);

    void can_setup();

    void packet_callback(leigh::jcan::Frame);

    template<typename T>
    double convert_scaled(const uint8_t *bytes, double max);

    template<typename T>
    T from_bytes(const uint8_t *bytes);

    template<typename T>
    void send_raw(uint32_t id, T data);

    template<typename T>
    void send_scaled(uint32_t id, double value, double max);
};

}  // namespace auger_hardware

#endif  // AUGER_HARDWARE__AUGER_HARDWARE_HPP_
