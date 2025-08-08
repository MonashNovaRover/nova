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
 * @authors Terry Tian
 */

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <limits>
#include <memory>
#include <queue>
#include <ranges>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"

#include "nova_controller_common/hardware_interface_wrapper.hpp"
#include "drive_controller_base/drive_controller_base.hpp"

namespace
{

constexpr auto DEFAULT_COMMAND_TOPIC = "/cmd_vel";
constexpr auto DEFAULT_COMMAND_OUT_TOPIC = "~/cmd_vel_out";

}  // namespace

namespace drive_controller_base
{

using namespace std::chrono_literals;
using namespace nova_controller_common;
using controller_interface::interface_configuration_type;
using controller_interface::InterfaceConfiguration;
using geometry_msgs::msg::TwistStamped;
using hardware_interface::HW_IF_POSITION;
using hardware_interface::HW_IF_VELOCITY;
using lifecycle_msgs::msg::State;

DriveControllerBase::DriveControllerBase()
  : controller_interface::ControllerInterface()
  , DRIVE_COMMAND_TYPE_(HW_IF_VELOCITY)
  , PIVOT_COMMAND_TYPE_(HW_IF_POSITION)
  , PIVOTS_PER_SIDE_(2)  // two pivots per side: front and back
{
}

const char* DriveControllerBase::drive_feedback_type() const
{
  return base_params_->drive_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
}

const char* DriveControllerBase::pivot_feedback_type() const
{
  return base_params_->pivot_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
}

controller_interface::CallbackReturn DriveControllerBase::on_init()
{
  try
  {
    // Create the parameter listener and get the parameters
    base_param_listener_ = std::make_shared<ParamListener>(get_node());
    base_params_ = std::make_shared<Params>(base_param_listener_->get_params());
    derived().param_listener_ = std::make_shared<ParamListener>(get_node());
    derived().params_ = std::make_shared<Params>(derived().param_listener_->get_params());
  }
  catch (const std::exception& e)
  {
    fprintf(stderr, "Exception thrown during init stage with message: %s \n", e.what());
    return controller_interface::CallbackReturn::ERROR;
  }

  wheels_per_side_ = base_params_->left_drive_names.size();

  // Initialise hardware interface wrapper
  hwif_wrapper_ =
    std::make_unique<HardwareInterfaceWrapper>(get_node(), state_interfaces_, command_interfaces_);

  // Initialise odometry
  odometry_ = std::make_unique<DerivedOdometry>(get_node(), base_params_);

  return controller_interface::CallbackReturn::SUCCESS;
}

InterfaceConfiguration DriveControllerBase::command_interface_configuration() const
{
  std::vector<std::string> conf_names;
  for (size_t i = 0; i < wheels_per_side_; ++i)
  {
    conf_names.push_back(base_params_->left_drive_names[i] + "/" + DRIVE_COMMAND_TYPE_);
    conf_names.push_back(base_params_->right_drive_names[i] + "/" + DRIVE_COMMAND_TYPE_);
  }
  for (size_t i = 0; i < PIVOTS_PER_SIDE_; ++i)
  {
    conf_names.push_back(base_params_->left_pivot_names[i] + "/" + PIVOT_COMMAND_TYPE_);
    conf_names.push_back(base_params_->right_pivot_names[i] + "/" + PIVOT_COMMAND_TYPE_);
  }

  return {interface_configuration_type::INDIVIDUAL, conf_names};
}

InterfaceConfiguration DriveControllerBase::state_interface_configuration() const
{
  if (base_params_->open_loop)
  {
    return {interface_configuration_type::NONE, {}};
  }

  std::vector<std::string> conf_names;
  for (size_t i = 0; i < wheels_per_side_; ++i)
  {
    conf_names.push_back(base_params_->left_drive_names[i] + "/" + drive_feedback_type());
    conf_names.push_back(base_params_->right_drive_names[i] + "/" + drive_feedback_type());
  }
  for (size_t i = 0; i < PIVOTS_PER_SIDE_; ++i)
  {
    conf_names.push_back(base_params_->left_pivot_names[i] + "/" + pivot_feedback_type());
    conf_names.push_back(base_params_->right_pivot_names[i] + "/" + pivot_feedback_type());
  }

  return {interface_configuration_type::INDIVIDUAL, conf_names};
}

controller_interface::return_type DriveControllerBase::update(
  const rclcpp::Time& time, const rclcpp::Duration& period)
{
  auto logger = get_node()->get_logger();

  if (base_param_listener_->is_old(*base_params_))
  {
    *base_params_ = base_param_listener_->get_params();
    RCLCPP_INFO(logger, "Parameters were updated");
  }
  if (derived().param_listener_->is_old(*derived().params_))
  {
    *derived().params_ = derived().param_listener_->get_params();
    RCLCPP_INFO(logger, "Parameters were updated");
  }

  std::shared_ptr<TwistStamped> command_msg_ptr = *(received_twist_msg_ptr_.readFromRT());
  if (command_msg_ptr == nullptr)
  {
    RCLCPP_WARN(logger, "Received TwistStamped message was a nullptr.");
    return controller_interface::return_type::ERROR;
  }
  else if ((std::isnan(command_msg_ptr->twist.linear.x) ||
            std::isnan(command_msg_ptr->twist.angular.z)))
  {
    RCLCPP_WARN_SKIPFIRST_THROTTLE(
      logger, *get_node()->get_clock(), cmd_vel_timeout_.seconds() * 1000,
      "Command message contains NaNs. Not updating reference interfaces.");
    return controller_interface::return_type::OK;
  }

  // ####################### Process input ###############################
  // In manual operation, twist values are scalar values from -1.0 to 1.0,
  // where 1.0 is the maximum linear or angular velocity
  double linear_input = command_msg_ptr->twist.linear.x;
  double angular_input = command_msg_ptr->twist.angular.z;
  double linear_velocity, angular_velocity;
  bool turning_left;
  double speed, turning_radius;

  // Brake if cmd_vel has timed out, override the stored command
  const auto age_of_last_command = time - command_msg_ptr->header.stamp;
  if (age_of_last_command > cmd_vel_timeout_)
  {
    linear_velocity = 0.0;
    angular_velocity = 0.0;
    speed = 0.0;
    turning_radius = INFINITY;
  }
  else if (base_params_->autonomous_mode)
  {
    angular_velocity = angular_input;
    linear_velocity = linear_input;
    speed = linear_velocity == 0 ? std::abs(zero_radius_ * angular_velocity) : linear_input;

    limiter_speed_.limit(speed, previous_speeds_[0], previous_speeds_[1], period.seconds());
    limiter_angular_.limit(
      angular_velocity, previous_angular_velocities_[0], previous_angular_velocities_[1],
      period.seconds());

    turning_radius = get_radius_from_velocities(linear_velocity, angular_velocity);
    turning_left = turning_radius == 0 ? angular_input > 0 : turning_radius > 0;

    const auto [max_requested_angle, left] = limit_radius_by_pivots(
      turning_radius, turning_left, half_steering_track_, half_wheel_base_, limiter_pivot_,
      previous_left_pivot_positions_, previous_right_pivot_positions_, period.seconds());

    const double requested_angular = angular_velocity;
    limiter_angular_.limit(
      angular_velocity, previous_angular_velocities_[0], previous_angular_velocities_[1],
      period.seconds());
    if (angular_velocity != requested_angular)
    {
      limit_speed_and_radius_by_angular(
        speed, turning_radius, angular_velocity, zero_radius_, inner_radius_, limiter_speed_,
        previous_speeds_, period.seconds());
    }

    const auto& prev_positions =
      left ? previous_left_pivot_positions_ : previous_right_pivot_positions_;
    double limited_angle = max_requested_angle;
    limiter_pivot_.limit(
      limited_angle, prev_positions[0], prev_positions[1], prev_positions[2],
      base_params_->pivot_rate_tolerance);
    if (limited_angle != max_requested_angle)
    {
      speed = 0.0;  // wait for the pivot to be within tolerance before moving
      limiter_speed_.limit(speed, previous_speeds_[0], previous_speeds_[1], period.seconds());
    }

    linear_velocity = turning_radius == 0 ? 0.0 : speed;
    angular_velocity = get_angular_from_radius_and_speed(
      turning_radius, speed, turning_left, zero_radius_, inner_radius_);
  }
  else
  {
    // Manual operation: left stick controls speed and right stick controls the pivot angle
    // Process raw angular input through a curve to calculate the turning radius
    // Prioritise keeping turning radius over speed
    turning_radius = angular_input == 0
                     ? INFINITY
                     : base_params_->input_curve_factor *
                         ((1.0 / angular_input) - std::copysign(1, angular_input));
    turning_left = turning_radius == 0 ? angular_input > 0 : turning_radius > 0;

    limit_radius_by_pivots(
      turning_radius, turning_left, half_steering_track_, half_wheel_base_, limiter_pivot_,
      previous_left_pivot_positions_, previous_right_pivot_positions_, period.seconds());

    speed = linear_input * base_params_->speed.max_velocity;
    const double requested_speed = speed;
    limiter_speed_.limit(speed, previous_speeds_[0], previous_speeds_[1], period.seconds());
    if (speed != requested_speed)
    {
      RCLCPP_INFO(logger, "Speed limited to %.2f", speed);
    }
    RCLCPP_INFO(logger, "Received: Speed = %.2f, Turning radius = %f", speed, turning_radius);

    // Calculate the angular velocity based on the limited speed
    angular_velocity = get_angular_from_radius_and_speed(
      turning_radius, speed, turning_left, zero_radius_, inner_radius_);
    RCLCPP_INFO(logger, "Calculated angular velocity = %.2f", angular_velocity);

    const double requested_angular = angular_velocity;
    limiter_angular_.limit(
      angular_velocity, previous_angular_velocities_[0], previous_angular_velocities_[1],
      period.seconds());
    if (angular_velocity != requested_angular)
    {
      limit_speed_and_radius_by_angular(
        speed, turning_radius, angular_velocity, zero_radius_, inner_radius_, limiter_speed_,
        previous_speeds_, period.seconds());
    }
    linear_velocity = turning_radius == 0 ? 0 : speed;
  }

  // ################### Update and publish odometry #####################
  if (base_params_->open_loop)
  {
    odometry_->update_open_loop(linear_velocity, angular_velocity, time);
  }
  else
  {
    double left_drive_feedback_mean = 0.0;
    double right_drive_feedback_mean = 0.0;
    double left_pivot_feedback_mean = 0.0;
    double right_pivot_feedback_mean = 0.0;

    // Drive feedback (average left and right drive joints)
    for (size_t pos = 0; pos < wheels_per_side_; ++pos)
    {
      const auto left_feedback_op =
        hwif_wrapper_->get_optional(encoded_pos(pos, JointSide::LEFT, JointType::DRIVE));
      const auto right_feedback_op =
        hwif_wrapper_->get_optional(encoded_pos(pos, JointSide::RIGHT, JointType::DRIVE));
      if (!left_feedback_op.has_value() || !right_feedback_op.has_value())
      {
        RCLCPP_DEBUG(
          logger,
          "Failed to get feedback from hardware interfaces for left/right drive joints %zu, "
          "odometry cannot be updated. Ending current update loop early.",
          pos);
        return controller_interface::return_type::OK;
      }
      const double left_feedback = left_feedback_op.value();
      const double right_feedback = right_feedback_op.value();

      if (std::isnan(left_feedback) || std::isnan(right_feedback))
      {
        RCLCPP_ERROR(
          logger,
          "Received NaN feedback for left/right drive joints %zu, odometry cannot be updated. "
          "Ending current update loop early.",
          pos);
        return controller_interface::return_type::ERROR;
      }

      left_drive_feedback_mean += left_feedback;
      right_drive_feedback_mean += right_feedback;
    }
    // Pivot feedback (average left and right pivot joints in reference to the front)
    for (size_t pos = 0; pos < PIVOTS_PER_SIDE_; ++pos)
    {
      const auto left_feedback_op =
        hwif_wrapper_->get_optional(encoded_pos(pos, JointSide::LEFT, JointType::PIVOT));
      const auto right_feedback_op =
        hwif_wrapper_->get_optional(encoded_pos(pos, JointSide::RIGHT, JointType::PIVOT));
      if (!left_feedback_op.has_value() || !right_feedback_op.has_value())
      {
        RCLCPP_DEBUG(
          logger,
          "Failed to get feedback from hardware interfaces for left/right pivot joints %zu, "
          "odometry cannot be updated. Ending current update loop early.",
          pos);
        return controller_interface::return_type::OK;
      }
      const double left_feedback = left_feedback_op.value();
      const double right_feedback = right_feedback_op.value();

      if (std::isnan(left_feedback) || std::isnan(right_feedback))
      {
        RCLCPP_ERROR(
          logger,
          "Received NaN feedback for left/right pivot joints %zu, odometry cannot be updated. "
          "Ending current update loop early.",
          pos);
        return controller_interface::return_type::ERROR;
      }

      const int multiplier =
        (pos == 0) ? 1 : -1;  // front pivot is positive, back pivot is negative
      left_pivot_feedback_mean += multiplier * left_feedback;
      right_pivot_feedback_mean += multiplier * right_feedback;
    }

    left_drive_feedback_mean /= wheels_per_side_;
    right_drive_feedback_mean /= wheels_per_side_;
    left_pivot_feedback_mean /= PIVOTS_PER_SIDE_;
    right_pivot_feedback_mean /= PIVOTS_PER_SIDE_;

    odometry_->update(
      left_drive_feedback_mean, right_drive_feedback_mean, left_pivot_feedback_mean,
      right_pivot_feedback_mean, time);
  }
  odometry_->publish(time);

  // ######################### Send commands #############################
  const double left_angle = get_pivot_angle_from_radius(
    turning_radius, true, turning_left, half_steering_track_, half_wheel_base_);
  const double right_angle = get_pivot_angle_from_radius(
    turning_radius, false, turning_left, half_steering_track_, half_wheel_base_);
  double left_ratio = get_speed_ratio(
    turning_radius, true, half_steering_track_, half_wheel_base_, zero_radius_, inner_radius_);
  double right_ratio = get_speed_ratio(
    turning_radius, false, half_steering_track_, half_wheel_base_, zero_radius_, inner_radius_);
  const double left_speed = speed * left_ratio;
  const double right_speed = speed * right_ratio;
  const double left_velocity = left_speed / base_params_->wheel_radius;
  const double right_velocity = right_speed / base_params_->wheel_radius;

  RCLCPP_INFO(logger, "speed ratios: left = %.2f, right = %.2f", left_ratio, right_ratio);

  // Set command values for drive
  for (size_t pos = 0; pos < wheels_per_side_; ++pos)
  {
    if (
      !hwif_wrapper_->set_value(
        left_velocity, encoded_pos(pos, JointSide::LEFT, JointType::DRIVE)) ||
      !hwif_wrapper_->set_value(
        right_velocity, encoded_pos(pos, JointSide::RIGHT, JointType::DRIVE)))
    {
      RCLCPP_ERROR(logger, "Failed to set drive command values for position %zu.", pos);
      return controller_interface::return_type::ERROR;
    }
  }
  // Set command values for pivots
  for (size_t pos = 0; pos < PIVOTS_PER_SIDE_; ++pos)
  {
    const double multiplier = (pos == 0) ? 1 : -1;  // invert angles for back pivots
    if (
      !hwif_wrapper_->set_value(
        multiplier * left_angle, encoded_pos(pos, JointSide::LEFT, JointType::PIVOT)) ||
      !hwif_wrapper_->set_value(
        multiplier * right_angle, encoded_pos(pos, JointSide::RIGHT, JointType::PIVOT)))
    {
      RCLCPP_ERROR(logger, "Failed to set pivot command values for position %zu.", pos);
      return controller_interface::return_type::ERROR;
    }
  }

  RCLCPP_DEBUG(
    logger, "Set drive commands: left_speed = %.2f, right_speed = %.2f", left_speed, right_speed);
  RCLCPP_DEBUG(
    logger, "Set pivot commands: left_angle = %.2f, right_angle = %.2f", left_angle, right_angle);
  RCLCPP_DEBUG(
    logger, "------------------------------------------------------------------------------------");

  // Update the previous command values for limiting
  previous_speeds_.pop_front();
  previous_speeds_.push_back(speed);
  previous_angular_velocities_.pop_front();
  previous_angular_velocities_.push_back(angular_velocity);
  previous_left_pivot_positions_.pop_front();
  previous_left_pivot_positions_.push_back(left_angle);
  previous_right_pivot_positions_.pop_front();
  previous_right_pivot_positions_.push_back(right_angle);
  /**
   * Derived classes should update the previous command values for limiting here, e.g.
   * previous_linear_velocities_.pop_front();
   * previous_linear_velocities_.push_back(linear_velocity);
   * previous_angular_velocities_.pop_front();
   * previous_angular_velocities_.push_back(angular_velocity);
   * previous_left_pivot_positions_.pop_front();
   * previous_left_pivot_positions_.push_back(left_angle);
   * previous_right_pivot_positions_.pop_front();
   * previous_right_pivot_positions_.push_back(right_angle);
   */

  // Publish commanded velocities
  if (base_params_->publish_commanded_velocities && realtime_commanded_twist_publisher_->trylock())
  {
    auto& commanded_twist_command = realtime_commanded_twist_publisher_->msg_;
    commanded_twist_command.header.stamp = time;
    commanded_twist_command.twist.linear.x = linear_velocity;
    commanded_twist_command.twist.linear.y = 0.0;
    commanded_twist_command.twist.linear.z = 0.0;
    commanded_twist_command.twist.angular.x = 0.0;
    commanded_twist_command.twist.angular.y = 0.0;
    commanded_twist_command.twist.angular.z = angular_velocity;
    realtime_commanded_twist_publisher_->unlockAndPublish();
  }

  return controller_interface::return_type::OK;
}

controller_interface::CallbackReturn DriveControllerBase::on_configure(
  const rclcpp_lifecycle::State&)
{
  auto logger = get_node()->get_logger();

  // update parameters if they have changed
  if (base_param_listener_->is_old(*base_params_))
  {
    *base_params_ = base_param_listener_->get_params();
    RCLCPP_INFO(logger, "Base parameters were updated");
  }
  if (derived().param_listener_->is_old(*derived().params_))
  {
    *derived().params_ = derived().param_listener_->get_params();
    RCLCPP_INFO(logger, "Parameters were updated");
  }

  if (base_params_->left_drive_names.size() != base_params_->right_drive_names.size())
  {
    RCLCPP_ERROR(
      logger, "The number of left wheels [%zu] and the number of right wheels [%zu] are different",
      base_params_->left_drive_names.size(), base_params_->right_drive_names.size());
    return controller_interface::CallbackReturn::ERROR;
  }

  if (base_params_->left_drive_names.empty())
  {
    RCLCPP_ERROR(logger, "Wheel names parameters are empty!");
    return controller_interface::CallbackReturn::ERROR;
  }

  if (base_params_->left_pivot_names.size() != 2 || base_params_->right_pivot_names.size() != 2)
  {
    RCLCPP_ERROR(
      logger, "Expected exactly two pivots per side, instead got %zu left and %zu right pivots",
      base_params_->left_pivot_names.size(), base_params_->right_pivot_names.size());
    return controller_interface::CallbackReturn::ERROR;
  }

  cmd_vel_timeout_ = rclcpp::Duration::from_seconds(base_params_->cmd_vel_timeout);

  limiter_speed_ = SpeedLimiter(
    base_params_->speed.has_velocity_limits, base_params_->speed.has_acceleration_limits,
    base_params_->speed.has_jerk_limits, base_params_->speed.min_velocity,
    base_params_->speed.max_velocity, base_params_->speed.min_acceleration,
    base_params_->speed.max_acceleration, base_params_->speed.min_jerk,
    base_params_->speed.max_jerk);
  limiter_angular_ = SpeedLimiter(
    base_params_->angular.has_velocity_limits, base_params_->angular.has_acceleration_limits,
    base_params_->angular.has_jerk_limits, base_params_->angular.min_velocity,
    base_params_->angular.max_velocity, base_params_->angular.min_acceleration,
    base_params_->angular.max_acceleration, base_params_->angular.min_jerk,
    base_params_->angular.max_jerk);
  limiter_pivot_ = PositionLimiter(
    base_params_->pivot.has_velocity_limits, base_params_->pivot.has_acceleration_limits,
    base_params_->pivot.has_jerk_limits, base_params_->pivot.min_velocity,
    base_params_->pivot.max_velocity, base_params_->pivot.min_acceleration,
    base_params_->pivot.max_acceleration, base_params_->pivot.min_jerk,
    base_params_->pivot.max_jerk);

  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }

  // Initialise twist publisher
  if (base_params_->publish_commanded_velocities)
  {
    commanded_twist_publisher_ = get_node()->create_publisher<TwistStamped>(
      DEFAULT_COMMAND_OUT_TOPIC, rclcpp::SystemDefaultsQoS());
    realtime_commanded_twist_publisher_ =
      std::make_shared<realtime_tools::RealtimePublisher<TwistStamped>>(commanded_twist_publisher_);
  }

  // Initialise twist subscriber
  twist_subscriber_ = get_node()->create_subscription<TwistStamped>(
    DEFAULT_COMMAND_TOPIC, rclcpp::SystemDefaultsQoS(),
    [this](const std::shared_ptr<TwistStamped> msg) -> void
    {
      if (!is_active_)
      {
        RCLCPP_WARN_ONCE(
          get_node()->get_logger(), "Can't accept new commands, subscriber is inactive");
        return;
      }
      if ((msg->header.stamp.sec == 0) && (msg->header.stamp.nanosec == 0))
      {
        RCLCPP_WARN_ONCE(
          get_node()->get_logger(),
          "Received TwistStamped with zero timestamp, setting it to current "
          "time, this message will only be shown once");
        msg->header.stamp = get_node()->get_clock()->now();
      }

      const auto current_time_diff = get_node()->now() - msg->header.stamp;

      if (
        cmd_vel_timeout_ == rclcpp::Duration::from_seconds(0.0) ||
        current_time_diff < cmd_vel_timeout_)
      {
        received_twist_msg_ptr_.writeFromNonRT(msg);
      }
      else
      {
        RCLCPP_WARN(
          get_node()->get_logger(),
          "Ignoring the received message (timestamp %.10f) because it is older than "
          "the current time by %.10f seconds, which exceeds the allowed timeout (%.4f)",
          rclcpp::Time(msg->header.stamp).seconds(), current_time_diff.seconds(),
          cmd_vel_timeout_.seconds());
      }
    });

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn DriveControllerBase::on_activate(
  const rclcpp_lifecycle::State&)
{
  // Configure joints
  std::vector<Joint> joints;
  for (size_t pos = 0; pos < wheels_per_side_; ++pos)
  {
    std::string left_drive_name = base_params_->left_drive_names[pos];
    std::string right_drive_name = base_params_->right_drive_names[pos];
    joints.emplace_back(
      left_drive_name, drive_feedback_type(), DRIVE_COMMAND_TYPE_,
      encoded_pos(pos, JointSide::LEFT, JointType::DRIVE));
    joints.emplace_back(
      right_drive_name, drive_feedback_type(), DRIVE_COMMAND_TYPE_,
      encoded_pos(pos, JointSide::RIGHT, JointType::DRIVE));
  }
  for (size_t pos = 0; pos < PIVOTS_PER_SIDE_; ++pos)
  {
    std::string left_pivot_name = base_params_->left_pivot_names[pos];
    std::string right_pivot_name = base_params_->right_pivot_names[pos];
    joints.emplace_back(
      left_pivot_name, pivot_feedback_type(), PIVOT_COMMAND_TYPE_,
      encoded_pos(pos, JointSide::LEFT, JointType::PIVOT));
    joints.emplace_back(
      right_pivot_name, pivot_feedback_type(), PIVOT_COMMAND_TYPE_,
      encoded_pos(pos, JointSide::RIGHT, JointType::PIVOT));
  }

  if (!hwif_wrapper_->configure_joint_handles(joints, base_params_->open_loop))
  {
    RCLCPP_ERROR(get_node()->get_logger(), "Error configuring drives and pivots");
    return controller_interface::CallbackReturn::ERROR;
  }

  is_halted_ = false;
  is_active_ = true;

  RCLCPP_INFO(get_node()->get_logger(), "Subscriber and publisher are now active.");
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn DriveControllerBase::on_deactivate(
  const rclcpp_lifecycle::State&)
{
  is_active_ = false;
  halt();
  reset_buffers();

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn DriveControllerBase::on_cleanup(const rclcpp_lifecycle::State&)
{
  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn DriveControllerBase::on_error(const rclcpp_lifecycle::State&)
{
  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

bool DriveControllerBase::reset()
{
  odometry_->reset();
  reset_buffers();
  twist_subscriber_.reset();
  is_active_ = false;

  return true;
}

void DriveControllerBase::reset_buffers()
{
  hwif_wrapper_->reset_handles();

  /**
   * Derived classes may override this method to reset their own buffers, e.g.
   * previous_speeds_ = {0.0, 0.0};
   * previous_angular_velocities_ = {0.0, 0.0};
   * previous_left_pivot_positions_ = {0.0, 0.0, 0.0};
   * previous_right_pivot_positions_ = {0.0, 0.0, 0.0};
   */

  // Fill RealtimeBuffer with NaNs so it will contain a known value
  // but still indicate that no command has yet been sent.
  received_twist_msg_ptr_.reset();
  std::shared_ptr<TwistStamped> empty_twist_ptr = std::make_shared<TwistStamped>();
  empty_twist_ptr->header.stamp = get_node()->now();
  empty_twist_ptr->twist.linear.x = std::numeric_limits<double>::quiet_NaN();
  empty_twist_ptr->twist.linear.y = std::numeric_limits<double>::quiet_NaN();
  empty_twist_ptr->twist.linear.z = std::numeric_limits<double>::quiet_NaN();
  empty_twist_ptr->twist.angular.x = std::numeric_limits<double>::quiet_NaN();
  empty_twist_ptr->twist.angular.y = std::numeric_limits<double>::quiet_NaN();
  empty_twist_ptr->twist.angular.z = std::numeric_limits<double>::quiet_NaN();
  received_twist_msg_ptr_.writeFromNonRT(empty_twist_ptr);
}

controller_interface::CallbackReturn DriveControllerBase::on_shutdown(
  const rclcpp_lifecycle::State&)
{
  return controller_interface::CallbackReturn::SUCCESS;
}

void DriveControllerBase::halt()
{
  // Send zero commands to all wheels and pivots
  for (size_t pos = 0; pos < wheels_per_side_; ++pos)
  {
    hwif_wrapper_->set_value(0.0, encoded_pos(pos, JointSide::LEFT, JointType::DRIVE));
    hwif_wrapper_->set_value(0.0, encoded_pos(pos, JointSide::RIGHT, JointType::DRIVE));
  }
  for (size_t pos = 0; pos < PIVOTS_PER_SIDE_; ++pos)
  {
    hwif_wrapper_->set_value(0.0, encoded_pos(pos, JointSide::LEFT, JointType::PIVOT));
    hwif_wrapper_->set_value(0.0, encoded_pos(pos, JointSide::RIGHT, JointType::PIVOT));
  }
}

}  // namespace drive_controller_base
