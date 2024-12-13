// Copyright 2020 PAL Robotics S.L.
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

/*
 * Author: Bailey Chessum
 */

#ifndef GENERIC_BROADCASTER__GENERIC_BROADCASTER_HPP_
#define GENERIC_BROADCASTER__GENERIC_BROADCASTER_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "controller_interface/controller_interface.hpp"
#include "generic_broadcaster/visibility_control.h"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "hardware_interface/handle.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_box.h"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"
#include "tf2_msgs/msg/tf_message.hpp"
#include "std_msgs/msg/float64.hpp"

#include "generic_broadcaster_parameters.hpp"

namespace generic_broadcaster {

class GenericBroadcaster : public controller_interface::ControllerInterface {
public:
    GENERIC_BROADCASTER_PUBLIC
    GenericBroadcaster();

    [[nodiscard]] GENERIC_BROADCASTER_PUBLIC
    controller_interface::InterfaceConfiguration command_interface_configuration() const override;

    [[nodiscard]] GENERIC_BROADCASTER_PUBLIC
    controller_interface::InterfaceConfiguration state_interface_configuration() const override;

    GENERIC_BROADCASTER_PUBLIC
    controller_interface::return_type update(const rclcpp::Time &time, const rclcpp::Duration &period) override;

    GENERIC_BROADCASTER_PUBLIC
    controller_interface::CallbackReturn

    on_init() override;

    GENERIC_BROADCASTER_PUBLIC
    controller_interface::CallbackReturn

    on_configure(const rclcpp_lifecycle::State &previous_state) override;

    GENERIC_BROADCASTER_PUBLIC
    controller_interface::CallbackReturn

    on_activate(const rclcpp_lifecycle::State &previous_state) override;

    GENERIC_BROADCASTER_PUBLIC
    controller_interface::CallbackReturn

    on_deactivate(const rclcpp_lifecycle::State &previous_state) override;

    GENERIC_BROADCASTER_PUBLIC
    controller_interface::CallbackReturn

    on_cleanup(const rclcpp_lifecycle::State &previous_state) override;

    GENERIC_BROADCASTER_PUBLIC
    controller_interface::CallbackReturn

    on_error(const rclcpp_lifecycle::State &previous_state) override;

    GENERIC_BROADCASTER_PUBLIC
    controller_interface::CallbackReturn

    on_shutdown(const rclcpp_lifecycle::State &previous_state) override;

protected:
    // Keep track of handles?
    std::optional<std::reference_wrapper<const hardware_interface::LoanedStateInterface>> state;

    // Publisher for the state
    using StatePublisher = realtime_tools::RealtimePublisher<std_msgs::msg::Float64>;
    rclcpp::Publisher<std_msgs::msg::Float64>::SharedPtr state_publisher_;
    std::unique_ptr<StatePublisher> realtime_state_publisher_;

    // Parameters from ROS for generic_broadcaster
    std::shared_ptr<ParamListener> param_listener_;
    Params params_;

    // publish rate limiter
    double publish_rate_ = 50.0;
    rclcpp::Duration publish_period_ = rclcpp::Duration::from_nanoseconds(0);
    rclcpp::Time previous_publish_timestamp_{0, 0, RCL_CLOCK_UNINITIALIZED};

    bool reset();
};

}  // namespace generic_broadcaster
#endif  // GENERIC_BROADCASTER__GENERIC_BROADCASTER_HPP_
