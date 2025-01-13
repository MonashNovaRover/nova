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

#ifndef NIR_PROBE_HARDWARE__NIR_PROBE_HARDWARE_HPP_
#define NIR_PROBE_HARDWARE__NIR_PROBE_HARDWARE_HPP_

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

namespace nir_probe_hardware
{

struct CommandInterface {
    std::optional<double> command;
};

struct StateInterface {
    std::optional<double> state;
};

enum class NIRSendCommand {
    NIR_PROBE_LED1_ON = 0x01, // Also turn LED2 off
    NIR_PROBE_LED2_ON = 0x02, // Also turn LED1 off
    NIR_PROBE_LED_OFF = 0x03,
    NIR_PROBE_READ_P1 = 0x04,
    NIR_PROBE_READ_P2 = 0x05,
};

enum class CanIdPrefix{
    NIR_PROBE_ID = 0x0F0,
    CARD_ID_RECEIVE_PD1 = 0x4F0,
    CARD_ID_RECEIVE_PD2 = 0x4F1,
};

enum class TelemetryPacket{
    PACKET_1,
    PACKET_2,
};

class NIRProbeHardware : public hardware_interface::SystemInterface
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
    std::string NIRProbeHarwareLoggerName;

    StateInterface nir_probe_led_state_;
    StateInterface ice_probe_led_state_;
    StateInterface water_probe_led_state_;

    std::unique_ptr<leigh::jcan::Bus> bus_;
    std::basic_string<char> can_device_;

    uint32_t can_id_;

    uint32_t clock_rate_;

    bool set_control_interface(const hardware_interface::InterfaceInfo & interface_info, bool command);

    bool stop_interface(const std::string &interface);

    bool start_interface(const std::string &interface);

    void can_setup();

    void packet_1_callback(leigh::jcan::Frame);

    void packet_2_callback(leigh::jcan::Frame);

    template<typename T>
    double convert_scaled(const uint8_t *bytes, double max);

    template<typename T>
    T from_bytes(const uint8_t *bytes);

    template<typename T>
    void send_raw(uint32_t id, T data);

    template<typename T>
    void send_scaled(uint32_t id, double value, double max);
};

}  // namespace nir_probe_hardware

#endif  // NIR_PROBE_HARDWARE__NIR_PROBE_HARDWARE_HPP_
