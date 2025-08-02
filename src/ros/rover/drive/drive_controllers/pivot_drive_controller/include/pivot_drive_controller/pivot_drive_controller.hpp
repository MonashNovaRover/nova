// Copyright (c) 2025 Monash Nova Rover
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

/**
 * @brief Controller for a four wheel steering rover.
 * @authors Terry Tian
 */

#ifndef PIVOT_DRIVE_CONTROLLER__PIVOT_DRIVE_CONTROLLER_HPP_
#define PIVOT_DRIVE_CONTROLLER__PIVOT_DRIVE_CONTROLLER_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <deque>
#include <string>
#include <vector>
#include <utility>

#include "controller_interface/controller_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "std_srvs/srv/empty.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"

#include "nova_controller_common/speed_limiter.hpp"
#include "nova_controller_common/position_limiter.hpp"
#include "nova_controller_common/hardware_interface_wrapper.hpp"
#include "pivot_drive_controller/odometry.hpp"
#include "pivot_drive_controller_parameters.hpp"

namespace pivot_drive_controller
{

class PivotDriveController : public controller_interface::ControllerInterface
{
public:
  PivotDriveController();

  controller_interface::InterfaceConfiguration command_interface_configuration() const override;
  controller_interface::InterfaceConfiguration state_interface_configuration() const override;

  controller_interface::return_type update(
    const rclcpp::Time& time, const rclcpp::Duration& period) override;

  controller_interface::CallbackReturn on_init() override;

  controller_interface::CallbackReturn on_configure(
    const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_activate(
    const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_deactivate(
    const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_cleanup(
    const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_error(
    const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_shutdown(
    const rclcpp_lifecycle::State& previous_state) override;

private:
  const char* drive_feedback_type() const;
  const char* pivot_feedback_type() const;

  bool reset();
  void reset_buffers();
  void halt();

  bool is_halted_ = false;

  // Parameters from ROS for pivot_drive_controller
  std::shared_ptr<ParamListener> param_listener_;
  std::shared_ptr<Params> params_;

  // Radius of the circle the rover makes with its wheels when turning on the spot
  double zero_radius_;
  // Turning radius at which the circle that the wheel to the side of the turn makes has the
  // same radius. Any turning radius less than this will cause the circle that the wheel
  // to the side of the turn makes to have a larger radius than the turning radius
  double inner_radius_;
  // Offset angle for the pivot joints, used to calculate the pivot angles
  double offset_angle_;
  double half_wheel_base_;
  double half_steering_track_;
  size_t num_wheels_per_side_;
  const size_t NUM_PIVOTS_PER_SIDE_;

  std::unique_ptr<nova_controller_common::HardwareInterfaceWrapper> hwif_wrapper_;
  std::unique_ptr<Odometry> odometry_;

  std::deque<double> previous_speeds_;                 // last two speed commands
  std::deque<double> previous_angular_velocities_;     // last two angular velocity commands
  std::deque<double> previous_left_pivot_positions_;   // last three left pivot position commands
  std::deque<double> previous_right_pivot_positions_;  // last three right pivot position commands

  // Limiters
  nova_controller_common::SpeedLimiter limiter_speed_;
  nova_controller_common::SpeedLimiter limiter_angular_;
  nova_controller_common::PositionLimiter limiter_pivot_;

  // Timeout to consider cmd_vel commands old
  rclcpp::Duration cmd_vel_timeout_ = rclcpp::Duration::from_seconds(0.5);

  // Realtime buffer for received TwistStamped messages
  bool is_active_ = false;
  rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr twist_subscriber_;
  realtime_tools::RealtimeBuffer<std::shared_ptr<geometry_msgs::msg::TwistStamped>>
    received_twist_msg_ptr_;

  const char* DRIVE_COMMAND_TYPE_;
  const char* PIVOT_COMMAND_TYPE_;
};

}  // namespace pivot_drive_controller

#endif  // PIVOT_DRIVE_CONTROLLER__PIVOT_DRIVE_CONTROLLER_HPP_
