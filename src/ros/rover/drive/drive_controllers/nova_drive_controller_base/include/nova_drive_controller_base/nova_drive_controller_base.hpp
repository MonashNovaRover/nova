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
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Monash Nova Rover Team
 * 
 * BRIEF: Base class for drive controllers.
 * 
 * Encapsulates common functionality for drive controllers, such as:
 * - subscribing to input
 * - publishing commanded velocities
 * - odometry
 * - managing params
 * - sending commands to hardware interfaces
 * - etc.
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * PLUGIN: nova_drive_controller_base
 * TOPICS:
 *  - subscriber:  /cmd_vel       [geometry_msgs/msg/TwistStamped]
 *  - publisher:   ~/cmd_vel_out  [geometry_msgs/msg/TwistStamped]
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * PACKAGE:   nova_drive_controller_base
 * AUTHORS:	  Terry Tian
 * CREATION:  2025
 * EDITED:    2025
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#ifndef NOVA_DRIVE_CONTROLLER_BASE__NOVA_DRIVE_CONTROLLER_BASE_HPP_
#define NOVA_DRIVE_CONTROLLER_BASE__NOVA_DRIVE_CONTROLLER_BASE_HPP_

#include <cstddef>
#include <chrono>
#include <memory>
#include <deque>
#include <string>
#include <vector>

#include "controller_interface/controller_interface.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"
#include "drive_interfaces/srv/drive_status.hpp"
#include "drive_interfaces/msg/drive_command.hpp"

#include "nova_controller_common/speed_limiter.hpp"
#include "nova_controller_common/velocity_limiter.hpp"
#include "nova_controller_common/position_limiter.hpp"
#include "nova_controller_common/hardware_interface_wrapper.hpp"
#include "nova_drive_controller_base/odometry.hpp"
#include "nova_drive_controller_base_parameters.hpp"

namespace nova_drive_controller_base
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

/**
 * Encodes the position, side, and type of a joint into a unique index for use
 * with HardwareInterfaceWrapper.
 * 
 * @param pos The position of the joint (0-indexed).
 * @param side The side of the joint (LEFT or RIGHT).
 * @param type The type of the joint (DRIVE or PIVOT).
 * @return A unique index for the joint.
 */
constexpr size_t encoded_pos(const size_t pos, const JointSide side, const JointType type)
{
  return pos << 2 | (static_cast<size_t>(side) << 1) | static_cast<size_t>(type);
}

struct Commands
{
  double linear_velocity_x;
  double linear_velocity_y;
  double angular_velocity;
  std::vector<double> left_drive_speeds;      // left drive speeds
  std::vector<double> right_drive_speeds;     // right drive speeds
  std::vector<double> left_pivot_positions;   // left pivot positions
  std::vector<double> right_pivot_positions;  // right pivot positions
};

class NovaDriveControllerBase : public controller_interface::ControllerInterface
{
public:
  NovaDriveControllerBase();

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

protected:
  /**
   * Initialises parameters for the drive controller.
   */
  virtual void init_params() = 0;
  
  /**
   * Updates parameters for the drive controller.
   */
  virtual void update_params() = 0;
  
  /**
   * Resets the limiter buffers, e.g. previous_speeds_.
   */
  virtual void reset_limiter_buffers() = 0;
  
  /**
   * Main method that derived classes must implement. Converts a Twist message
   * to commands and data to be sent to the hardware interfaces and be used in odometry
   * respectively.
   * 
   * @param twist_msg The Twist message to convert.
   * @param autonomous_mode Whether the controller is in autonomous mode.
   * @param period The period of the update.
   * @return A Commands struct containing commands and data.
   */
  virtual Commands twist_to_commands(
    const geometry_msgs::msg::Twist& twist_msg, bool autonomous_mode,
    const rclcpp::Duration& period) = 0;

  /**
   * Calculates the turning radius through from the angular input through a curve.
   * 
   * @param angular_input The angular input to calculate the turning radius from.
   * @return The turning radius in meters.
   */
  double turning_radius_from_angular_input(double angular_input) const;

  // Parameters from ROS for nova_drive_controller_base
  std::shared_ptr<ParamListener> base_param_listener_;
  std::shared_ptr<Params> base_params_;

  size_t wheels_per_side_;
  const size_t PIVOTS_PER_SIDE_;

  /**
   * Derived clases will declare their own buffers, e.g.
   * std::deque<double> previous_speeds_;
   */

  // Limiters
  nova_controller_common::SpeedLimiter limiter_drive_;
  nova_controller_common::VelocityLimiter limiter_drive_velocity_;
  nova_controller_common::SpeedLimiter limiter_angular_;
  nova_controller_common::PositionLimiter limiter_pivot_;

  std::atomic<bool> hold_position_ = false;

private:
  const char* drive_feedback_type() const;
  const char* pivot_feedback_type() const;

  bool reset();
  virtual void reset_buffers();
  void halt();

  const char* DRIVE_COMMAND_TYPE_;
  const char* PIVOT_COMMAND_TYPE_;

  const std::string DEFAULT_COMMAND_TOPIC_;
  const std::string DEFAULT_COMMAND_OUT_TOPIC_;

  bool is_active_ = false;
  bool is_halted_ = false;

  std::unique_ptr<nova_controller_common::HardwareInterfaceWrapper> hwif_wrapper_;
  std::unique_ptr<Odometry> odometry_;

  // Timeout to consider cmd_vel commands old
  rclcpp::Duration cmd_vel_timeout_ = rclcpp::Duration::from_seconds(0.5);

  // Subscriber and realtime buffer for received TwistStamped messages
  rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr twist_subscriber_;
  realtime_tools::RealtimeBuffer<std::shared_ptr<geometry_msgs::msg::TwistStamped>>
    received_twist_msg_ptr_;

  // Publisher and realtime buffer for commanded TwistStamped messages
  std::shared_ptr<rclcpp::Publisher<drive_interfaces::msg::DriveCommand>> command_publisher_;
  std::shared_ptr<realtime_tools::RealtimePublisher<drive_interfaces::msg::DriveCommand>>
    realtime_command_publisher_;

  rclcpp::Service<drive_interfaces::srv::DriveStatus>::SharedPtr set_drive_status_service_;

};

}  // namespace nova_drive_controller_base

#endif  // NOVA_DRIVE_CONTROLLER_BASE__NOVA_DRIVE_CONTROLLER_BASE_HPP_
