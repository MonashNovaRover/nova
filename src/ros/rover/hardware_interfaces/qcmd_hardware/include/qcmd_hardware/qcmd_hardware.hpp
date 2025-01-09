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

#ifndef QCMD_HARDWARE__QCMD_HARDWARE_HPP_
#define QCMD_HARDWARE__QCMD_HARDWARE_HPP_

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

namespace qcmd_hardware
{

struct ControlInterface {
    std::optional<double> command;
    double max {std::numeric_limits<double>::quiet_NaN()};
};


// From: https://www.notion.so/MNR-CANBUS-Standards-9dc47508ed3e4dfda2aa9ae97fe1ad54#a52b20fa4b874f1ab0bf73e6fa9dfc6c

enum class QCMDCommandID {
    CONTROL = 0x0,              // Data is sent as a 16 bit signed integer
    CURRENT_LIMIT = 0x2,        // Data is sent as a 7 bit unsigned integer
};

// This info is in .xacro file now
enum class QCMDMotorID {
    PUMP_CLEANING = 0x011,      // 'NOT being used'
    PUMP_MIX_TO_SHOT = 0x012,
    AUGER_ACTUATION = 0x021,
    AUGER_DRILL_SPIN = 0x022,
    PUMP_SHOT_TO_SPEC = 0x031,
    ANALYSIS_ARM_ACTUATION = 0x032,
    MIXER_1 = 0x041,
    MIXER_2 = 0x042,
};

enum class QCMDCommandData {
    MAX_CURRENT = 0x7F,
    MAX_MAGNITUDE = 0x7FFF, 
    FORWARD_DIRECTION = 1,
    REVERSE_DIRECTION = -1,
};

class QCMDHardware : public hardware_interface::SystemInterface
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

private:
    std::string QCMDHardwareLoggerName;

    ControlInterface hw_velocity_;
    ControlInterface hw_current_;

    std::unique_ptr<leigh::jcan::Bus> bus_;
    std::basic_string<char> can_device_;

    uint32_t can_id_;

    bool set_control_interface(const hardware_interface::InterfaceInfo & interface_info, bool command);

    template<typename T>
    double convert_scaled(const uint8_t *bytes, double max);

    template<typename T>
    T from_bytes(const uint8_t *bytes);

    template<typename T>
    void send_raw(uint32_t id, T data);

    template<typename T>
    void send_scaled(uint32_t id, double value, double max);
};

}  // namespace qcmd_hardware

#endif  // QCMD_HARDWARE__QCMD_HARDWARE_HPP_
