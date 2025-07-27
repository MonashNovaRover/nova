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
 * ROS conventions dictate that left = positive and right = negative.
 * The received twist messages from teleop follow this convention.
 * The pivots are zeroed at an offset angle to enable pivot drive with +-90 degree position limits.
 * The offset angle is applied in the hardware interface wrapper, so 0 degrees is straight ahead in
 * this controller.
 * 
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
#include "pivot_drive_controller/kinematics.hpp"
#include "pivot_drive_controller/pivot_drive_controller.hpp"

namespace
{

constexpr auto DEFAULT_INPUT_TOPIC = "/cmd_vel";

}  // namespace

namespace pivot_drive_controller
{

using namespace std::chrono_literals;
using namespace nova_controller_common;
using controller_interface::interface_configuration_type;
using controller_interface::InterfaceConfiguration;
using geometry_msgs::msg::TwistStamped;
using hardware_interface::HW_IF_POSITION;
using hardware_interface::HW_IF_VELOCITY;
using lifecycle_msgs::msg::State;

PivotDriveController::PivotDriveController()
  : controller_interface::ControllerInterface()
  , DRIVE_COMMAND_TYPE(HW_IF_VELOCITY)
  , PIVOT_COMMAND_TYPE(HW_IF_POSITION)
{
}

const char* PivotDriveController::drive_feedback_type() const
{
  return params_->drive_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
}

const char* PivotDriveController::pivot_feedback_type() const
{
  return params_->pivot_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
}

controller_interface::CallbackReturn PivotDriveController::on_init()
{
  try
  {
    // Create the parameter listener and get the parameters
    param_listener_ = std::make_shared<ParamListener>(get_node());
    params_ = std::make_shared<Params>(param_listener_->get_params());
  }
  catch (const std::exception& e)
  {
    fprintf(stderr, "Exception thrown during init stage with message: %s \n", e.what());
    return controller_interface::CallbackReturn::ERROR;
  }

  half_wheel_base_ = params_->wheel_base / 2;
  half_steering_track_ = params_->steering_track / 2;

  zero_radius_ = std::hypot(half_wheel_base_, half_steering_track_);
  RCLCPP_INFO_STREAM(get_node()->get_logger(), "zero_radius_: " << zero_radius_);

  // Angle at which the wheels are initially offset
  offset_angle_ = atan(params_->steering_track / params_->wheel_base);
  RCLCPP_INFO_STREAM(get_node()->get_logger(), "offset_angle: " << offset_angle_);

  // Initialise hardware interface wrapper
  hwif_wrapper_ = std::make_unique<HardwareInterfaceWrapper>(
    get_node(), offset_angle_, state_interfaces_, command_interfaces_);

  // Initialise odometry
  odometry_ = std::make_unique<Odometry>(get_node(), params_);

  return controller_interface::CallbackReturn::SUCCESS;
}

InterfaceConfiguration PivotDriveController::command_interface_configuration() const
{
  std::vector<std::string> conf_names;
  for (const std::string& joint_pos : params_->joints)
  {
    conf_names.push_back(
      params_->drive_names.joints_map.at(joint_pos).value + "/" + DRIVE_COMMAND_TYPE);

    conf_names.push_back(
      params_->pivot_names.joints_map.at(joint_pos).value + "/" + PIVOT_COMMAND_TYPE);
  }

  return {interface_configuration_type::INDIVIDUAL, conf_names};
}

InterfaceConfiguration PivotDriveController::state_interface_configuration() const
{
  if (params_->open_loop)
  {
    return {interface_configuration_type::NONE, {}};
  }

  std::vector<std::string> conf_names;
  for (const std::string& joint_pos : params_->joints)
  {
    conf_names.push_back(
      params_->drive_names.joints_map.at(joint_pos).value + "/" + drive_feedback_type());

    conf_names.push_back(
      params_->pivot_names.joints_map.at(joint_pos).value + "/" + pivot_feedback_type());
  }

  return {interface_configuration_type::INDIVIDUAL, conf_names};
}

controller_interface::return_type PivotDriveController::update(
  const rclcpp::Time& time, const rclcpp::Duration& period)
{
  if (param_listener_->is_old(*params_))
  {
    *params_ = param_listener_->get_params();
    RCLCPP_INFO(get_node()->get_logger(), "Parameters were updated");
  }

  std::shared_ptr<TwistStamped> command_msg_ptr = *(received_twist_msg_ptr_.readFromRT());
  if (command_msg_ptr == nullptr)
  {
    RCLCPP_WARN(get_node()->get_logger(), "Received TwistStamped message was a nullptr.");
    return controller_interface::return_type::ERROR;
  }
  else if ((std::isnan(command_msg_ptr->twist.linear.x) ||
            std::isnan(command_msg_ptr->twist.angular.z)))
  {
    RCLCPP_WARN_SKIPFIRST_THROTTLE(
      get_node()->get_logger(), *get_node()->get_clock(), cmd_vel_timeout_.seconds() * 1000,
      "Command message contains NaNs. Not updating reference interfaces.");
    return controller_interface::return_type::OK;
  }

  // ####################### Process input ###############################
  // In manual operation, twist values are scalar values from -1.0 to 1.0,
  // where 1.0 is the maximum linear or angular velocity
  double linear_input = command_msg_ptr->twist.linear.x;
  double angular_input = command_msg_ptr->twist.angular.z;
  double linear_velocity, angular_velocity;
  bool turning_left = angular_input > 0;
  double speed, turning_radius;
  // Brake if cmd_vel has timed out, override the stored command
  const auto age_of_last_command = time - command_msg_ptr->header.stamp;
  if (age_of_last_command > cmd_vel_timeout_)
  {
    linear_velocity = 0.0;
    speed = 0.0;
    turning_radius = INFINITY;
  }
  else if (params_->autonomous_mode)
  {
    linear_velocity = linear_input;
    angular_velocity = angular_input;

    limiter_linear_.limit(
      linear_velocity, previous_linear_velocities_[0], previous_linear_velocities_[1],
      period.seconds());
    limiter_angular_.limit(
      angular_velocity, previous_angular_velocities_[0], previous_angular_velocities_[1],
      period.seconds());

    turning_radius = get_radius_from_velocities(linear_velocity, angular_velocity);

    const auto prev_positions =
      turning_left ? previous_left_pivot_positions_ : previous_right_pivot_positions_;
    limit_radius_by_pivot(
      turning_radius, turning_left, half_steering_track_, half_wheel_base_, limiter_pivot_,
      prev_positions[0], prev_positions[1], prev_positions[2], period.seconds());

    speed = linear_velocity == 0 ? std::abs(zero_radius_ * angular_velocity) : linear_velocity;
  }
  else
  {
    // Manual operation: left stick controls speed and right stick controls the pivot angle
    // Process raw angular input through a curve to calculate the turning radius
    // Prioritise keeping turning radius over speed
    turning_radius =
      angular_input == 0
        ? INFINITY
        : params_->curve_factor * ((1.0 / angular_input) - std::copysign(1, angular_input));

    // const auto prev_positions =
    //   turning_left ? previous_left_pivot_positions_ : previous_right_pivot_positions_;
    // limit_radius_by_pivot(
    //   turning_radius, turning_left, half_steering_track_, half_wheel_base_, limiter_pivot_,
    //   prev_positions[0], prev_positions[1], prev_positions[2], period.seconds());
    
    speed = linear_input * params_->linear.max_velocity;
    linear_velocity = turning_radius == 0 ? 0 : speed;

    RCLCPP_INFO_THROTTLE(
      get_node()->get_logger(), *get_node()->get_clock(), 500,
      "Received: Speed = %.2f, Linear velocity = %.2f, Turning radius = %f", speed, linear_velocity,
      turning_radius);

    double temp = linear_velocity;
    limiter_linear_.limit(
      linear_velocity, previous_linear_velocities_[0], previous_linear_velocities_[1],
      period.seconds());
    if (linear_velocity != temp)
    {
      speed = linear_velocity;
      RCLCPP_INFO_THROTTLE(
        get_node()->get_logger(), *get_node()->get_clock(), 500, "Speed limited to %.2f", speed);
    }
    // Calculate the angular velocity based on the limited speed
    angular_velocity =
      get_angular_from_radius_and_speed(turning_radius, speed, turning_left, zero_radius_);
    RCLCPP_INFO_THROTTLE(
      get_node()->get_logger(), *get_node()->get_clock(), 500, "Calculated angular velocity = %.2f",
      angular_velocity);

    temp = angular_velocity;
    limiter_angular_.limit(
      angular_velocity, previous_angular_velocities_[0], previous_angular_velocities_[1],
      period.seconds());
    if (angular_velocity != temp)
    {
      const bool keep_speed = false;  // temporary toggle for testing
      if (keep_speed)
      {
        // Keep speed and recalculate turning radius to match angular velocity
        turning_radius = angular_velocity == 0 ? INFINITY : speed / angular_velocity;
        RCLCPP_INFO_THROTTLE(
          get_node()->get_logger(), *get_node()->get_clock(), 500,
          "Angular velocity limited to %.2f, recalculating turning radius to %.2f",
          angular_velocity, turning_radius);
      }
      else
      {
        // Recalculate (decrease) the speed to match the limited angular velocity
        std::tie(speed, linear_velocity) =
          turning_radius == 0
            ? std::make_tuple(std::copysign(zero_radius_ * angular_velocity, linear_input), 0.0)
            : std::make_tuple(
                std::copysign(turning_radius * angular_velocity, linear_velocity), speed);
        temp = linear_velocity;
        limiter_linear_.limit(
          linear_velocity, previous_linear_velocities_[0], previous_linear_velocities_[1],
          period.seconds());
        if (linear_velocity != temp)
        {
          // If the new speed was limited, we need to recalculate the turning radius as well
          // because we can't decrease the speed further
          speed = linear_velocity;
          turning_radius = angular_velocity == 0 ? INFINITY : speed / angular_velocity;
          RCLCPP_INFO_THROTTLE(
            get_node()->get_logger(), *get_node()->get_clock(), 500,
            "Angular velocity limited to %.2f, recalculating speed to %.2f and turning radius to "
            "%.2f",
            angular_velocity, speed, turning_radius);
        }
        else
        {
          RCLCPP_INFO_THROTTLE(
            get_node()->get_logger(), *get_node()->get_clock(), 500,
            "Angular velocity limited to %.2f, recalculating speed to %.2f", angular_velocity,
            speed);
        }
      }
    }
  }

  // ################### Update and publish odometry #####################
  if (params_->open_loop)
  {
    odometry_->update_open_loop(linear_velocity, angular_velocity, time);
  }
  else
  {
    double left_drive_feedback_mean = 0.0;
    double right_drive_feedback_mean = 0.0;
    double left_pivot_feedback_mean = 0.0;
    double right_pivot_feedback_mean = 0.0;

    for (const std::string& joint_pos_str : params_->joints)
    {
      const JointPosition joint_pos = to_joint_position(joint_pos_str);
      const auto drive_feedback_op = hwif_wrapper_->get_optional(joint_pos, JointType::DRIVE);
      const auto pivot_feedback_op = hwif_wrapper_->get_optional(joint_pos, JointType::PIVOT);
      if (!drive_feedback_op.has_value() || !pivot_feedback_op.has_value())
      {
        RCLCPP_DEBUG(
          get_node()->get_logger(),
          "Failed to get feedback from hardware interfaces for %s joint, odometry cannot be "
          "updated. Ending current update loop early.",
          joint_pos_str.c_str());
        return controller_interface::return_type::OK;
      }
      const double drive_feedback = drive_feedback_op.value();
      const double pivot_feedback = pivot_feedback_op.value();

      if (std::isnan(drive_feedback) || std::isnan(pivot_feedback))
      {
        RCLCPP_ERROR(
          get_node()->get_logger(),
          "Received NaN feedback for %s joint, odometry cannot be updated. Ending current update "
          "loop early.",
          joint_pos_str.c_str());
        return controller_interface::return_type::ERROR;
      }

      if (joint_pos == JointPosition::FRONT_LEFT || joint_pos == JointPosition::BACK_LEFT)
      {
        left_drive_feedback_mean += drive_feedback;
        left_pivot_feedback_mean +=
          (joint_pos == JointPosition::BACK_LEFT ? -1 : 1) * pivot_feedback;
      }
      else
      {
        right_drive_feedback_mean += drive_feedback;
        right_pivot_feedback_mean +=
          (joint_pos == JointPosition::BACK_LEFT ? -1 : 1) * pivot_feedback;
      }
    }

    left_drive_feedback_mean /= 2;
    right_drive_feedback_mean /= 2;
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
  const double left_ratio =
    get_speed_ratio(turning_radius, true, half_steering_track_, half_wheel_base_);
  const double right_ratio =
    get_speed_ratio(turning_radius, false, half_steering_track_, half_wheel_base_);
  const double left_speed = speed * left_ratio;
  const double right_speed = speed * right_ratio;
  const double left_velocity = left_speed / params_->wheel_radius;
  const double right_velocity = right_speed / params_->wheel_radius;

  // Set command values for drive
  if (
    !hwif_wrapper_->set_value(left_velocity, JointPosition::FRONT_LEFT, JointType::DRIVE) ||
    !hwif_wrapper_->set_value(left_velocity, JointPosition::BACK_LEFT, JointType::DRIVE) ||
    !hwif_wrapper_->set_value(right_velocity, JointPosition::FRONT_RIGHT, JointType::DRIVE) ||
    !hwif_wrapper_->set_value(right_velocity, JointPosition::BACK_RIGHT, JointType::DRIVE))
  {
    RCLCPP_ERROR(get_node()->get_logger(), "Failed to set drive command values.");
    return controller_interface::return_type::ERROR;
  }
  // Set command values for pivots
  if (
    !hwif_wrapper_->set_value(left_angle, JointPosition::FRONT_LEFT, JointType::PIVOT) ||
    !hwif_wrapper_->set_value(-left_angle, JointPosition::BACK_LEFT, JointType::PIVOT) ||
    !hwif_wrapper_->set_value(right_angle, JointPosition::FRONT_RIGHT, JointType::PIVOT) ||
    !hwif_wrapper_->set_value(-right_angle, JointPosition::BACK_RIGHT, JointType::PIVOT))
  {
    RCLCPP_ERROR(get_node()->get_logger(), "Failed to set pivot command values.");
    return controller_interface::return_type::ERROR;
  }

  RCLCPP_INFO_THROTTLE(
    get_node()->get_logger(), *get_node()->get_clock(), 500,
    "Set drive commands: left_speed = %.2f, right_speed = %.2f", left_speed, right_speed);
  RCLCPP_INFO_THROTTLE(
    get_node()->get_logger(), *get_node()->get_clock(), 500,
    "Set pivot commands: left_angle = %.2f, right_angle = %.2f", left_angle, right_angle);

  // Update the previous command values for limiting
  previous_linear_velocities_.pop_front();
  previous_linear_velocities_.push_back(linear_velocity);
  previous_angular_velocities_.pop_front();
  previous_angular_velocities_.push_back(angular_velocity);
  previous_left_pivot_positions_.pop_front();
  previous_left_pivot_positions_.push_back(left_angle);
  previous_right_pivot_positions_.pop_front();
  previous_right_pivot_positions_.push_back(right_angle);

  return controller_interface::return_type::OK;
}

controller_interface::CallbackReturn PivotDriveController::on_configure(
  const rclcpp_lifecycle::State&)
{
  cmd_vel_timeout_ = rclcpp::Duration::from_seconds(params_->cmd_vel_timeout);

  limiter_linear_ = SpeedLimiter(
    params_->linear.has_velocity_limits, params_->linear.has_acceleration_limits,
    params_->linear.has_jerk_limits, params_->linear.min_velocity, params_->linear.max_velocity,
    params_->linear.min_acceleration, params_->linear.max_acceleration, params_->linear.min_jerk,
    params_->linear.max_jerk);
  limiter_angular_ = SpeedLimiter(
    params_->angular.has_velocity_limits, params_->angular.has_acceleration_limits,
    params_->angular.has_jerk_limits, params_->angular.min_velocity, params_->angular.max_velocity,
    params_->angular.min_acceleration, params_->angular.max_acceleration, params_->angular.min_jerk,
    params_->angular.max_jerk);
  limiter_pivot_ = PositionLimiter(
    params_->pivot.has_velocity_limits, params_->pivot.has_acceleration_limits,
    params_->pivot.has_jerk_limits, params_->pivot.min_velocity, params_->pivot.max_velocity,
    params_->pivot.min_acceleration, params_->pivot.max_acceleration, params_->pivot.min_jerk,
    params_->pivot.max_jerk);

  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }

  // Initialize twist subscriber
  twist_subscriber_ = get_node()->create_subscription<TwistStamped>(
    DEFAULT_INPUT_TOPIC, rclcpp::SystemDefaultsQoS(),
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

controller_interface::CallbackReturn PivotDriveController::on_activate(
  const rclcpp_lifecycle::State&)
{
  // Configure joints
  std::vector<Joint> joints;
  for (const std::string& joint_pos_str : params_->joints)
  {
    JointPosition joint_pos = to_joint_position(joint_pos_str);

    joints.emplace_back(
      params_->drive_names.joints_map.at(joint_pos_str).value, drive_feedback_type(),
      DRIVE_COMMAND_TYPE, joint_pos, JointType::DRIVE);

    joints.emplace_back(
      params_->pivot_names.joints_map.at(joint_pos_str).value, pivot_feedback_type(),
      PIVOT_COMMAND_TYPE, joint_pos, JointType::PIVOT);
  }

  if (!hwif_wrapper_->configure_joint_handles(joints, params_->open_loop))
  {
    RCLCPP_ERROR(get_node()->get_logger(), "Error configuring drives and pivots");
    return controller_interface::CallbackReturn::ERROR;
  }

  is_halted_ = false;
  is_active_ = true;

  RCLCPP_INFO(get_node()->get_logger(), "Subscriber and publisher are now active.");
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn PivotDriveController::on_deactivate(
  const rclcpp_lifecycle::State&)
{
  is_active_ = false;
  halt();
  reset_buffers();

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn PivotDriveController::on_cleanup(
  const rclcpp_lifecycle::State&)
{
  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn PivotDriveController::on_error(const rclcpp_lifecycle::State&)
{
  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

bool PivotDriveController::reset()
{
  odometry_->reset();
  reset_buffers();
  twist_subscriber_.reset();
  is_active_ = false;

  return true;
}

void PivotDriveController::reset_buffers()
{
  hwif_wrapper_->reset_handles();

  previous_linear_velocities_ = {0.0, 0.0};
  previous_angular_velocities_ = {0.0, 0.0};
  previous_left_pivot_positions_ = {0.0, 0.0, 0.0};
  previous_right_pivot_positions_ = {0.0, 0.0, 0.0};

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

controller_interface::CallbackReturn PivotDriveController::on_shutdown(
  const rclcpp_lifecycle::State&)
{
  return controller_interface::CallbackReturn::SUCCESS;
}

void PivotDriveController::halt()
{
  for (const auto& joint_pos :
       {JointPosition::FRONT_LEFT, JointPosition::FRONT_RIGHT, JointPosition::BACK_LEFT,
        JointPosition::BACK_RIGHT})
  {
    for (const auto& joint_type : {JointType::DRIVE, JointType::PIVOT})
    {
      hwif_wrapper_->set_value(0.0, joint_pos, joint_type);
    }
  }
}

}  // namespace pivot_drive_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
  pivot_drive_controller::PivotDriveController, controller_interface::ControllerInterface)
