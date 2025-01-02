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
 * Author: Bence Magyar, Enrique Fernández, Manuel Meraz
 */

#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "generic_broadcaster/generic_broadcaster.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"
#include "tf2/LinearMath/Quaternion.h"

namespace {
    constexpr auto DEFAULT_STATE_TOPIC = "~/generic/state";
}  // namespace

namespace generic_broadcaster {
using namespace std::chrono_literals;
using controller_interface::interface_configuration_type;
using controller_interface::InterfaceConfiguration;
using hardware_interface::HW_IF_POSITION;
using hardware_interface::HW_IF_VELOCITY;
using lifecycle_msgs::msg::State;

GenericBroadcaster::GenericBroadcaster() : controller_interface::ControllerInterface() {}

controller_interface::CallbackReturn GenericBroadcaster::on_init() {
    try {
        // Create the parameter listener and get the parameters
        param_listener_ = std::make_shared<ParamListener>(get_node());
        params_ = param_listener_->get_params();
    }
    catch (const std::exception &e) {
        fprintf(stderr, "Exception thrown during init stage with message: %s \n", e.what());
        return controller_interface::CallbackReturn::ERROR;
    }

    return controller_interface::CallbackReturn::SUCCESS;
}

InterfaceConfiguration GenericBroadcaster::command_interface_configuration() const {
    std::vector<std::string> conf_names;

    // Put names of command interfaces you want here
    // But we only support state interfaces atm

    return {interface_configuration_type::INDIVIDUAL, conf_names};
}

InterfaceConfiguration GenericBroadcaster::state_interface_configuration() const {
    std::vector<std::string> conf_names;

    // Put whatever state interfaces you want to claim here
    //conf_names.emplace_back("FrontLeftWheel/test/value");

    return {interface_configuration_type::INDIVIDUAL, conf_names};
}

controller_interface::return_type GenericBroadcaster::update(
        const rclcpp::Time &time, const rclcpp::Duration &period) {
    auto logger = get_node()->get_logger();
    if (get_lifecycle_state().id() == State::PRIMARY_STATE_INACTIVE) {
        return controller_interface::return_type::OK;
    }

    // update parameters if they have changed
    if (param_listener_->is_old(params_)) {
        params_ = param_listener_->get_params();
        RCLCPP_INFO(logger, "Parameters were updated");
    }

    // Read the state interface
    const double feedback = state.value().get().get_value();

    // validate
    if (std::isnan(feedback)) {
        RCLCPP_ERROR(logger, "GenericHardware feedback was NaN");
            return controller_interface::return_type::ERROR;
    }

    // Publish feedback with the publisher
    if (realtime_state_publisher_->trylock()) {
        auto& msg = realtime_state_publisher_->msg_;

        msg.data = feedback;

        realtime_state_publisher_->unlockAndPublish();
    }

    return controller_interface::return_type::OK;
}

controller_interface::CallbackReturn GenericBroadcaster::on_configure(const rclcpp_lifecycle::State &) {
    auto logger = get_node()->get_logger();

    // update parameters if they have changed
    if (param_listener_->is_old(params_)) {
        params_ = param_listener_->get_params();
        RCLCPP_INFO(logger, "Parameters were updated");
    }
    if (!reset()) {
        return controller_interface::CallbackReturn::ERROR;
    }

    // initialize odometry publisher and messasge
    state_publisher_ = get_node()->create_publisher<std_msgs::msg::Float64>(
        DEFAULT_STATE_TOPIC, rclcpp::SystemDefaultsQoS());
    realtime_state_publisher_ = std::make_unique<realtime_tools::RealtimePublisher<std_msgs::msg::Float64>>(state_publisher_);

    // limit the publication on the topics /odom and /tf
    publish_rate_ = params_.publish_rate;
    publish_period_ = rclcpp::Duration::from_seconds(1.0 / publish_rate_);

    return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn GenericBroadcaster::on_activate(
        const rclcpp_lifecycle::State &) {

    // when we can't get the state or command interfaces we want, ERROR
    if (state_interfaces_.empty()) {
        RCLCPP_ERROR(
            get_node()->get_logger(),
            "Unable to find the state interface for GenericPublisher");
        return controller_interface::CallbackReturn::ERROR;
    }

    // find state interfaces of importance, keep reference
    state = state_interfaces_[0];

    RCLCPP_DEBUG(get_node()->get_logger(), "Subscriber and publisher are now active.");
    return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn GenericBroadcaster::on_deactivate(
        const rclcpp_lifecycle::State &) {
    // TODO: forget about state
    state.reset();

    return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn GenericBroadcaster::on_cleanup(
        const rclcpp_lifecycle::State &) {
    if (!reset()) {
        return controller_interface::CallbackReturn::ERROR;
    }

    // Set the received ros2 topic messages to be defaults
    // TODO: Set defaults
    // received_twist_msg_ptr_.set(std::make_shared<geometry_msgs::msg::TwistStamped>());

    return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn GenericBroadcaster::on_error(const rclcpp_lifecycle::State &) {
    if (!reset()) {
        return controller_interface::CallbackReturn::ERROR;
    }
    return controller_interface::CallbackReturn::SUCCESS;
}

bool GenericBroadcaster::reset() {

    // Forget about the registered handles
    state.reset();

    // Reset the subscribers

    // Reset the received messages to nullptr

    return true;
}

controller_interface::CallbackReturn GenericBroadcaster::on_shutdown(
        const rclcpp_lifecycle::State &) {
    return controller_interface::CallbackReturn::SUCCESS;
}

}  // namespace generic_broadcaster

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
        generic_broadcaster::GenericBroadcaster, controller_interface::ControllerInterface)
