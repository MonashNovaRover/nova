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

#ifndef CMD_HARDWARE__CMD_HARDWARE_HPP_
#define CMD_HARDWARE__CMD_HARDWARE_HPP_

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

namespace cmd_hardware
{
// TODO: Find how the PID_TUNE command works, and make this correct. This is just from BLCMD hardware.
struct PIConfig {
    int16_t kp;
    int16_t ki_ts;
    int16_t max_threshold;
};

struct ControlInterface {
    std::optional<double> command;
    std::optional<double> state;
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

enum class CMDSendCommand {
    STOP                = 0x0,    // Turns off the all the motor outputs.
    TWITCH_FORWARD      = 0x1,    // Powers the motor forward at roughly 90% power. Used for easy debugging
    TWITCH_BACKWARDS    = 0x2,    // Powers the motor in reverse at roughly 90% power. Used for easy debugging
    // Drives the motor in open loop PWM mode. Takes in single signed integer. Sign dictates direction, magnitude
    // dictates duty cycle with the maximum value of 32767 being full power forward and the minimum of -32768 being full
    // power reverse.
    // Send with int16 data.
    PWM_DRIVE           = 0x3,
    // Drives the motor in closed loop velocity control mode. Takes in single signed integer. Sign dictates direction,
    // magnitude dictates velocity target with the maximum value of 32767 being full speed forward and the minimum of
    // -32768 being full speed reverse. For specific motors there is a max velocity target recommended is between 70%
    // and 90% to allow it to be achieved by the cmds without clipping.
    // Send with int16 data.
    PID_DRIVE           = 0x4,
    PID_TUNE            = 0x5,    // Complicated. Sets the pid constants. Only used for tuning.
};

enum class TelemetryPacket{
};

enum class CanIdPrefix{
    SEND = 0,
    RECEIVE = 4
};

enum class CMDReceiveCommand{
};

enum class CMDConfigCommand{
    HAS_RESOLVER    = 0x0,
    KP_CURRENT      = 0x1,
    KI_CURRENT      = 0x2,
    MAX_CURRENT     = 0x3,
    KP_VELOCITY     = 0x4,
    KI_VELOCITY     = 0x5,
    MAX_VELOCITY    = 0x6,
    MIN_INTERVAL    = 0x7,
    KP_POSITION     = 0x8,
    KI_POSITION     = 0x9,
    MAX_POSITION    = 0xA,
    PACKET_1_SPEED  = 0xB,
    PACKET_2_SPEED  = 0xC,
    PACKET_3_SPEED  = 0xD,
    PACKET_4_SPEED  = 0xE,
};

class CMDHardware : public hardware_interface::SystemInterface
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

        /// Unconfirmed. When true, the sign of all inputs and outputs are reversed.
        bool reversed = false;

        /// The maximum position in radians, to be mapped to the largest position in CAN; 0x7FFF. This is not a limit.
        double max_position = M_PI;

        /// The maximum velocity in radians per second, to be mapped to the largest velocity in CAN; 0x7FFF. This is not a limit.
        std::optional<double> max_velocity = std::nullopt;

        /// A reduction ratio resolver readings are scaled by.
        double resolver_reduction {std::numeric_limits<double>::quiet_NaN()};

        /// An offset to apply to all readings, in radians, such that it is added to resolver messages, and subtracted from commands
        double position_offset = 0.0;
    };

private:
    std::string CMDHardwareLoggerName;

    ControlInterface hw_velocity_;
    PositionInterface hw_position_;
    ControlInterface hw_effort_;

    ControlMode control_mode_;

    std::unique_ptr<leigh::jcan::Bus> bus_;

    Params params_;
    int reversed_multiplier_ = 1;

    hardware_interface::CallbackReturn apply_parameters();

    bool set_control_interface(const hardware_interface::InterfaceInfo & interface_info, bool command);

    bool stop_interface(const std::string &interface);

    bool start_interface(const std::string &interface);

    void can_setup();

    /// @brief      Get a configuration value from the CMD
    /// @param      command - The config value to get
    /// @returns    The optional with config value if received, empty optional otherwise
    template<typename T>
    std::optional<T> get_config(CMDConfigCommand command);

    /// @brief      Create the can ID for a given CMDSendCommand
    /// @param      command - The command to send
    /// @returns    The can ID
    uint32_t make_can_id(CMDSendCommand command) const;

    /// @brief      Create the can ID for a given CMDReceiveCommand
    /// @param      command - The command to send
    /// @returns    The can ID
    uint32_t make_can_id(CMDReceiveCommand command) const;

    /// @brief      Create the can ID for a given TelemetryPacket
    /// @param      command - The command to send
    /// @returns    The can ID
    uint32_t make_can_id(TelemetryPacket packet) const;

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

}  // namespace cmd_hardware

#endif  // CMD_HARDWARE__CMD_HARDWARE_HPP_
