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

#ifndef BLCMD_HARDWARE__BLCMD_HARDWARE_HPP_
#define BLCMD_HARDWARE__BLCMD_HARDWARE_HPP_

#include <string>
#include <vector>
#include <optional>
#include <bit>

#include "rclcpp/rclcpp.hpp"
#include "hardware_interface/system_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "hardware_interface/hardware_info.hpp"
#include "hardware_interface/types/hardware_interface_return_values.hpp"
#include "rclcpp/macros.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "transmission_interface/transmission.hpp"

#include "jcan/jcan.h"

namespace blcmd_hardware
{

#ifdef IS_BIG_ENDIAN

#define cpuToBE16(val) val
#define beToCPU16(val) val

#else

#define cpuToBE16(val) std::byteswap(val)
#define beToCPU16(val) std::byteswap(val)

#endif

// you must use the above macros to access
// these variables to ensure the byteorder
// is right
struct __attribute__((packed)) Telem1_t {
    int16_t velocity;
    int16_t Qcurrent;
};

static_assert(sizeof(struct Telem1_t) == 4);

struct __attribute__((packed)) Telem2_t {
    int16_t interval;
    int16_t Dcurrent;
};
static_assert(sizeof(struct Telem2_t) == 4);

struct __attribute__((packed)) Telem3_t {
    int16_t resPosition;
    int16_t resVelocity;
};
static_assert(sizeof(struct Telem3_t) == 4);

struct __attribute__((packed)) Telem4_t {
    int16_t power;
    int16_t voltage;
    int16_t temperature;
    int16_t current;
};
static_assert(sizeof(struct Telem4_t) == 8);


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

enum class BLCMDConfigCommand{
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

class BLCMDHardware : public hardware_interface::SystemInterface
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
    struct Params {
        /// The name of the CAN bus interface the target BLCMD is on. Should be 'can0', 'can1', or 'can2'.
        std::string candevice = "";

        /// The 2nd hexadecimal digit in the 12-bit CAN id used in messages to/from the BLCMD board.
        std::vector<uint32_t> canids;

        /// The clock rate of the blcmd's processor.
        uint32_t clock_rate = 100000000;

        /// The number of pulses per revolution of the incremental encoder. This changes if you flip the dip switches on
        /// the incremental encoder. The datasheet lists numbers for each quarter turn, so you multiply the number on the
        /// datasheet by 4
        uint16_t revolution_pulses = 2048*4;

        /// The gear ratio between the incremental encoder's rotation and the actual joint's rotation
        double gear_ratio = 1.0;

        /// If the motor is going at 0x7fff velocity, there are this many blcmd processor instruction clock cycles between
        /// pulses of the incremental encoder.
        uint16_t min_interval = 122;

        /// Unconfirmed. When true, the sign of all inputs and outputs are reversed.
        /// TODO: just make the gear ratio and resolver reduction negative?
        bool reversed = false;

        /// Unconfirmed. When true, the hardware interface will use min_interval from parameters. When false,
        /// min_interval is determined through requesting configuration from the BLCMD board.
        bool mock = false;

        /// Unconfirmed. When true, the hardware interface will ignore resolver data, and determine position through
        /// integrating velocity feedback.
        bool integrate_velocity = false;

        /// The maximum position in radians, to be mapped to the largest position in CAN; 0x7FFF. This is not a limit.
        std::optional<double> max_position = std::nullopt;
        
        /// The maximum velocity in radians per second, to be mapped to the largest velocity in CAN; 0x7FFF. This is not a limit.
        std::optional<double> max_velocity = std::nullopt;

        /// A reduction ratio resolver readings are scaled by.
        double resolver_reduction {std::numeric_limits<double>::quiet_NaN()};

        /// What the resolver reads when it is at the URDF's zero radians position.
        int16_t zero_offset = 0;

        /// How many integer ticks of the resolver value correspond to 360 degrees of revolution for the resolver
        int32_t res_ticks_per_rev = 0;

        bool diff_wrist = false;
        
    };

    std::string BLCMDHardwareLoggerName;

    std::vector<ControlInterface> hw_velocities_;
    std::vector<PositionInterface> hw_positions_;
    std::vector<ControlInterface> hw_efforts_;

    std::vector<std::shared_ptr<transmission_interface::Transmission>> transmissions_;

    struct InterfaceData
    {
      explicit InterfaceData(const std::string & name);

      std::string name_;
      double command_;
      double state_;

      double transmission_passthrough_;
    };

    std::vector<InterfaceData> joint_interfaces_;
    std::vector<InterfaceData> actuator_interfaces_;

    ControlMode control_mode_;

    std::unique_ptr<leigh::jcan::Bus> bus_;

    Params params_;
    int reversed_multiplier_ = 1;

    hardware_interface::CallbackReturn apply_parameters();

    bool set_control_interface(const hardware_interface::InterfaceInfo & interface_info, bool command, int index);

    bool stop_interface(const std::string &interface);

    bool start_interface(const std::string &interface);

    void can_setup();

    /// @brief      Get a configuration value from the BLCMD
    /// @param      command - The config value to get
    /// @returns    The optional with config value if received, empty optional otherwise
    template<typename T>
    std::optional<T> get_config(BLCMDConfigCommand command, uint32_t canid);

    /// @brief      Create the can ID for a given BLCMDSendCommand
    /// @param      command - The command to send
    /// @returns    The can ID
    uint32_t make_can_id(BLCMDSendCommand command, uint32_t canid) const;

    /// @brief      Create the can ID for a given BLCMDReceiveCommand
    /// @param      command - The command to send
    /// @returns    The can ID
    uint32_t make_can_id(BLCMDReceiveCommand command, uint32_t canid) const;

    /// @brief      Create the can ID for a given TelemetryPacket
    /// @param      command - The command to send
    /// @returns    The can ID
    uint32_t make_can_id(TelemetryPacket packet, uint32_t canid) const;

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

    static bool is_true(std::string& text);
};

}  // namespace blcmd_hardware

#endif  // BLCMD_HARDWARE__BLCMD_HARDWARE_HPP_
