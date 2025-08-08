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
 * @brief Abstract base class for drive controllers.
 * Uses CRTP (Curiously Recurring Template Pattern) to allow access to derived class' parameters.
 *
 * @authors Terry Tian
 */

#ifndef DRIVE_CONTROLLER_BASE__DRIVE_CONTROLLER_BASE_HPP_
#define DRIVE_CONTROLLER_BASE__DRIVE_CONTROLLER_BASE_HPP_

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
#include "drive_controller_base/odometry.hpp"
#include "drive_controller_base_parameters.hpp"

namespace drive_controller_base
{

enum class JointSide
{
  LEFT,
  RIGHT
};

enum class JointType
{
  DRIVE,
  PIVOT
};

constexpr size_t encoded_pos(const size_t pos, const JointSide side, const JointType type)
{
  return pos << 2 | (static_cast<size_t>(side) << 1) | static_cast<size_t>(type);
}

struct Commands
{
  double linear_velocity = 0.0;
  double angular_velocity = 0.0;
  double flw_speed = 0.0;     // front left wheel speed
  double blw_speed = 0.0;     // back right wheel speed
  double frw_speed = 0.0;     // front right wheel speed
  double brw_speed = 0.0;     // back left wheel speed
  double flp_position = 0.0;  // front left pivot position
  double blp_position = 0.0;  // back left pivot position
  double frp_position = 0.0;  // front right pivot position
  double brp_position = 0.0;  // back right pivot position

  Commands() = default;

  Commands(double linear, double angular, double left_pivot, double right_pivot)
    : linear_velocity(linear)
    , angular_velocity(angular)
    , left_pivot_position(left_pivot)
    , right_pivot_position(right_pivot)
  {
  }
};
}

template <typename Derived, typename DerivedOdometry>
class DriveControllerBase : public controller_interface::ControllerInterface
{
public:
  DriveControllerBase();

  controller_interface::InterfaceConfiguration command_interface_configuration() const override;
  controller_interface::InterfaceConfiguration state_interface_configuration() const override;

  virtual controller_interface::return_type update(
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

protected:
  const char* drive_feedback_type() const;
  const char* pivot_feedback_type() const;
  const char* DRIVE_COMMAND_TYPE_;
  const char* PIVOT_COMMAND_TYPE_;

  bool reset();
  virtual void reset_buffers();
  void halt();

  bool is_active_ = false;
  bool is_halted_ = false;

  // Parameters from ROS for drive_controller_base
  std::shared_ptr<ParamListener> base_param_listener_;
  std::shared_ptr<Params> base_params_;

  size_t wheels_per_side_;
  const size_t PIVOTS_PER_SIDE_;

  std::unique_ptr<nova_controller_common::HardwareInterfaceWrapper> hwif_wrapper_;
  std::unique_ptr<DerivedOdometry> odometry_;

  /**
   * Derived clases will declare their own buffers, e.g.
   * std::deque<double> previous_linear_velocities_;
   * std::deque<double> previous_angular_velocities_;
   * std::deque<double> previous_left_pivot_positions_;
   * std::deque<double> previous_right_pivot_positions_;
   */

  // Limiters
  nova_controller_common::SpeedLimiter limiter_speed_;
  nova_controller_common::SpeedLimiter limiter_angular_;
  nova_controller_common::PositionLimiter limiter_pivot_;

  // Timeout to consider cmd_vel commands old
  rclcpp::Duration cmd_vel_timeout_ = rclcpp::Duration::from_seconds(0.5);

  // Subscriber and realtime buffer for received TwistStamped messages
  rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr twist_subscriber_;
  realtime_tools::RealtimeBuffer<std::shared_ptr<geometry_msgs::msg::TwistStamped>>
    received_twist_msg_ptr_;

  // Publisher and realtime buffer for commanded TwistStamped messages
  std::shared_ptr<rclcpp::Publisher<geometry_msgs::msg::TwistStamped>> commanded_twist_publisher_;
  std::shared_ptr<realtime_tools::RealtimePublisher<geometry_msgs::msg::TwistStamped>>
    realtime_commanded_twist_publisher_;

private:
  const Derived& derived() const
  {
    return static_cast<const Derived&>(*this);
  }
};

}  // namespace drive_controller_base

#endif  // DRIVE_CONTROLLER_BASE__DRIVE_CONTROLLER_BASE_HPP_
