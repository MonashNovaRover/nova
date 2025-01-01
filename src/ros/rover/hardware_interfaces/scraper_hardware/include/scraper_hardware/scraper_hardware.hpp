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

#ifndef SCRAPER_HARDWARE__SCRAPER_HARDWARE_HPP_
#define SCRAPER_HARDWARE__SCRAPER_HARDWARE_HPP_

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

namespace SCRAPER_HARDWARE
{
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

// New commands for scraper

// 3 command interfaces (arm, bucket, scoop) and 1 state interface (limit switch)

enum class Scraper {
    CAN_BUS = "can1",

    // used to have "PERCENT" suffix
    ARM_MAX_VELOCITY_SCALING = 1.0,
    SCOOP_MAX_VELOCITY_SCALING = 1.0,
    BUCKET_MAX_VELOCITY_SCALING = 0.6,

    CAN_BUS_PARAM = "can_bus",
    CARD_TYPE_PARAM = "card_type",
    ARM_MAX_VEL_PERCENT_PARAM = "arm_max_vel_percent",
    SCOOP_MAX_VEL_PERCENT_PARAM = "scoop_max_vel_percent",
    BUCKET_MAX_VEL_PERCENT_PARAM = "bucket_max_vel_percent",
}

enum class ScraperJONO {
    // CAN IDs
    ARM_ID = 0x0A0,
    SCOOP_ID = 0x0A0,
    BUCKET_ID = 0x0A0,
    LIMIT_SWITCH_ID = 0x4A2,

    // first byte of data is this, then next byte of data determines magnitude of command (0 is minimum, FF is maximum)
    ARM_FORWARDS = 0x04,
    ARM_BACKWARDS = 0x03,
    SCOOP_FORWARDS = 0x06,
    SCOOP_BACKWARDS = 0x05,
    BUCKET_OPEN = 0x01,
    BUCKET_CLOSE = 0x02,

    BUCKET_LIMIT_SWITCH_CLOSED = 0x01, // old code has this being the first byte of data received (if not, then the whole frame is invalid and should be ignored)
    LIMIT_SWITCH_CLEAR = 0x00, // then if second byte is this, bucket can continue
    LIMIT_SWITCH_HIT = 0xFF, // if second byte is this, bucket should be halted. FROM OLD CODE: "used to be 0x01"
}

// original code is defaulting to using CMD cards
enum class ScraperCMD {
    ARM_ID = 0x0B3,
    SCOOP_ID = 0x0C3,
    BUCKET_ID = 0x0D3,

    ARM_FORWARDS = 1,
    ARM_BACKWARDS = -1,
    SCOOP_FORWARDS = 1,
    SCOOP_BACKWARDS = -1,
    BUCKET_CLOSE = 1,
    BUCKET_OPEN = -1,
}


// BLCMD commands below
enum class ScraperSendCommand {
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

enum class ScraperReceiveCommand{
    ERR_WARN_INF = 0x0,
    CONFIG_DATA = 0x9,
    WRITE_CONFIRMATION = 0xA
};

enum class ScraperConfigCommand{
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

class ScraperHardware : public hardware_interface::SystemInterface
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
    std::string ScraperHardwareLoggerName;

    ControlInterface hw_velocity_;
    PositionInterface hw_position_;
    ControlInterface hw_effort_;

    ControlMode control_mode_;

    std::unique_ptr<leigh::jcan::Bus> bus_;
    std::basic_string<char> can_device_;

    uint32_t can_id_;

    uint16_t min_interval_;

    uint32_t clock_rate_;

    uint16_t revolution_pulses_;

    double gear_ratio_;

    bool mock_ = false;

    bool integrate_velocity_ = false;

    int reversed_multiplier_ = 1;


    bool set_control_interface(const hardware_interface::InterfaceInfo & interface_info, bool command);

    bool stop_interface(const std::string &interface);

    bool start_interface(const std::string &interface);

    void can_setup();

    /// @brief      Get a configuration value from the Scraper
    /// @param      command - The config value to get
    /// @returns    The optional with config value if received, empty optional otherwise
    template<typename T>
    std::optional<T> get_config(ScraperConfigCommand command);

    /// @brief      Create the can ID for a given ScraperSendCommand
    /// @param      command - The command to send
    /// @returns    The can ID
    uint32_t make_can_id(ScraperSendCommand command) const;

    /// @brief      Create the can ID for a given ScraperReceiveCommand
    /// @param      command - The command to send
    /// @returns    The can ID
    uint32_t make_can_id(ScraperReceiveCommand command) const;

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

}  // namespace SCRAPER_HARDWARE

#endif  // SCRAPER_HARDWARE__SCRAPER_HARDWARE_HPP_
