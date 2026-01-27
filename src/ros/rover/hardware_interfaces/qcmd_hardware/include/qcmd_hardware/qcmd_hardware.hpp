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
    std::optional<double> state;
//    double max {std::numeric_limits<double>::quiet_NaN()};
};

struct EffortInterface : ControlInterface {
    /// This reference state does not include multi-turn emulation
    double reference_command = 0.0;
};

enum class TelemetryPacket {
    RESOLVER_ARBITRATION_ID = 0x0A0
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

    hardware_interface::return_type prepare_command_mode_switch(
        const std::vector<std::string> & start_interfaces,
        const std::vector<std::string> & stop_interfaces) override;

protected:
    struct Params {
        /// The name of the CAN bus interface the target BLCMD is on. Should be 'can0', 'can1', or 'can2'.
        std::basic_string<char> candevice = "";

        /// The 2nd hexadecimal digit in the 12-bit CAN id used in messages to/from the BLCMD board.
        uint32_t canid = 0;

        /// Unconfirmed. When true, the hardware interface will use min_interval from parameters. When false,
        /// min_interval is determined through requesting configuration from the BLCMD board.
        bool mock = false;

        /// When true, the sign of all velocity inputs and outputs on CAN are reversed.
        bool reverse_velocity = false;

        /// When true, the sign velocity feedback on CAN is reversed. This is applied on top of reverse_velocity.
        bool reverse_velocity_feedback = false;

        /// The maximum velocity in radians per second, to be mapped to the largest velocity in CAN; 0x7FFF. This is not a limit.
        double max_effort {std::numeric_limits<double>::quiet_NaN()};

        /// A reduction ratio resolver readings are scaled by.
        double resolver_reduction {std::numeric_limits<double>::quiet_NaN()};
    };

private:
    std::string QCMDHardwareLoggerName;

    EffortInterface hw_effort_;

    std::unique_ptr<leigh::jcan::Bus> bus_;

    Params params_;

    hardware_interface::CallbackReturn apply_parameters();

    bool set_control_interface(const hardware_interface::InterfaceInfo & interface_info, bool command);

    bool stop_interface(const std::string &interface);

    bool start_interface(const std::string &interface);

    void can_setup();

//    void packet_1_callback(leigh::jcan::Frame);
//
//    void packet_3_callback(leigh::jcan::Frame);

    template<typename T>
    double convert_scaled(const uint8_t *bytes, double max);

    template<typename T>
    T from_bytes(const uint8_t *bytes);

    template<typename T>
    void send_raw(uint32_t id, T data);

    template<typename T>
    void send_scaled(uint32_t id, double value, double max);

    static bool is_true(std::string& text);
};

}  // namespace qcmd_hardware

#endif  // qcmd_hardware__qcmd_hardware_HPP_
