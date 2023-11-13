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

#ifndef ROVER_HARDWARE__BLCMD_HARDWARE_HPP_
#define ROVER_HARDWARE__BLCMD_HARDWARE_HPP_

#include <string>
#include <vector>
#include <optional>

#include "rclcpp/rclcpp.hpp"
#include "hardware_interface/actuator_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp/macros.hpp"
#include "rclcpp_lifecycle/state.hpp"

#include "jcan/jcan.h"

namespace rover_hardware
{
constexpr const char * BLCMDHardwareLoggerName = "BLCMDHardware";

struct PIConfig {
    int16_t kp;
    int16_t ki_ts;
    int16_t max_threshold;
};

struct BLCMDConfig {
    bool has_resolver;
    PIConfig current_config;
    PIConfig velocity_config;
    PIConfig position_config;
    int16_t min_interval;
};

struct ControlInterface {
    std::optional<double> command;
    std::optional<double> state;
    double min {std::numeric_limits<double>::quiet_NaN()};
    double max {std::numeric_limits<double>::quiet_NaN()};
};

struct PositionInterface : ControlInterface {
    double resolver_reduction {std::numeric_limits<double>::quiet_NaN()};
};

enum class ControlMode {
    Undefined,
    Position,
    Velocity,
    Effort,
};

enum class BLCMDSendCommand {
    STOP                = 0x0,    // Disables all current through the motor, free spinning.
    FORWARD             = 0x1,    // Drive with FOC velocity control forward for 0.5s
    REVERSE             = 0x2,    // Drive with FOC velocity control forward for 0.5s
    DRIVE_VELOCITY      = 0x3,    // Drive with FOC velocity control at given signed integer speed
    DRIVE_POSITION      = 0x4,    // Drive with FOC position control to given angle. (-32,768 to +32,767) → (-π,π)
    DRIVE_CURRENT       = 0x5,    // Drive with FOC at selected current (torque)
    DRIVE_OPEN_LOOP     = 0x6,    // Drive open loop interpolating some set speed.
    HOME_ROTOR          = 0x7,    // Send request to home rotor
    ZERO_RESOLVER       = 0x8,    // Send request to zero resolver
    GET_CONFIG          = 0x9,    // Send request to get configuration
    SET_CONFIG          = 0xA     // Send request to set configuration
};

enum class TelemetryPacket{
    PACKET_1 = 0x1,
    PACKET_2,
    PACKET_3,
    PACKET_4
};

enum class CanIdPrefix{
    SEND = 0,
    RECEIVE = 4
};

enum class BLCMDReceiveCommand{
    ERR_WARN_INF = 0x0,
    CONFIG_DATA = 0x9,
    WRITE_CONFIRMATION = 0xA
};

class BLCMDHardware : public hardware_interface::ActuatorInterface
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
    ControlInterface hw_velocity_;
    PositionInterface hw_position_;
    ControlInterface hw_effort_;

    ControlMode control_mode_;

    std::unique_ptr<leigh::jcan::Bus> bus_;
    std::basic_string<char> can_device_;

    uint32_t can_id_;

    bool set_control_interface(const hardware_interface::InterfaceInfo & interface_info, bool command);

    void can_setup();

    /// @brief      Create the can ID for a given BLCMDSendCommand
    /// @param      command - The command to send
    /// @returns    The can ID
    uint32_t make_can_id(BLCMDSendCommand command);

    /// @brief      Create the can ID for a given BLCMDReceiveCommand
    /// @param      command - The command to send
    /// @returns    The can ID
    uint32_t make_can_id(BLCMDReceiveCommand command);

    /// @brief      Create the can ID for a given TelemetryPacket
    /// @param      command - The command to send
    /// @returns    The can ID
    uint32_t make_can_id(TelemetryPacket packet);

    void packet_1_callback(leigh::jcan::Frame);

    void packet_3_callback(leigh::jcan::Frame);

    template<typename T>
    double convert_scaled(const uint8_t *bytes, double max);

    template<typename T>
    T from_bytes(const uint8_t *bytes);

    template<typename T>
    void send_raw(uint32_t id, T data);

    template<typename T>
    void send_scaled(uint32_t id, double value, double max);
};

}  // namespace rover_hardware

#endif  // ROVER_HARDWARE__BLCMD_HARDWARE_HPP_
