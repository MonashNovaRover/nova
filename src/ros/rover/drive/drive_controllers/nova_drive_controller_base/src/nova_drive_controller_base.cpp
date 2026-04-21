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
 * PACKAGE:   nova_drive_controller_base
 * AUTHORS:	  Terry Tian
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

// Define colors for logging
#define C_INFO "\033[1;34m"    // blue
#define C_ERROR "\033[1;31m"   // red
#define C_WARN "\033[;33m"     // yellow
#define C_SUCCESS "\033[1;32m" // green
#define C_END "\033[0m"

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/rclcpp.hpp"

#include "nova_controller_common/hardware_interface_wrapper.hpp"
#include "nova_drive_controller_base/nova_drive_controller_base.hpp"

namespace nova_drive_controller_base
{

using namespace nova_controller_common;
using controller_interface::interface_configuration_type;
using controller_interface::InterfaceConfiguration;
using geometry_msgs::msg::Twist;
using geometry_msgs::msg::TwistStamped;
using hardware_interface::HW_IF_POSITION;
using hardware_interface::HW_IF_VELOCITY;
using lifecycle_msgs::msg::State;

NovaDriveControllerBase::NovaDriveControllerBase()
  : controller_interface::ControllerInterface()
  , PIVOTS_PER_SIDE_(2)  // two pivots per side: front and back
  , DRIVE_COMMAND_TYPE_(HW_IF_VELOCITY)
  , PIVOT_COMMAND_TYPE_(HW_IF_POSITION)
  , DEFAULT_COMMAND_TOPIC_("/cmd_vel")
  , DEFAULT_COMMAND_OUT_TOPIC_("~/cmd_vel_out")
{
}

double NovaDriveControllerBase::turning_radius_from_angular_input(double angular_input) const
{
  return angular_input == 0
         ? INFINITY
         : base_params_->input_curve_factor * ((1.0 / angular_input) - std::copysign(1, angular_input));
}

const char* NovaDriveControllerBase::drive_feedback_type() const
{
  return base_params_->drive_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
}

const char* NovaDriveControllerBase::pivot_feedback_type() const
{
  return base_params_->pivot_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
}

controller_interface::CallbackReturn NovaDriveControllerBase::on_init()
{
  try
  {
    // Create the parameter listener and get the parameters
    base_param_listener_ = std::make_shared<ParamListener>(get_node());
    base_params_ = std::make_shared<Params>(base_param_listener_->get_params());
    init_params();
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
  odometry_ = std::make_unique<Odometry>(get_node(), base_params_);

  last_received_time_ = rclcpp::Time(0, 0, get_node()->get_clock()->get_clock_type());

  return controller_interface::CallbackReturn::SUCCESS;
}

InterfaceConfiguration NovaDriveControllerBase::command_interface_configuration() const
{
  std::vector<std::string> conf_names;
  for (size_t pos = 0; pos < wheels_per_side_; ++pos)
  {
    conf_names.push_back(base_params_->left_drive_names[pos] + "/" + DRIVE_COMMAND_TYPE_);
    conf_names.push_back(base_params_->right_drive_names[pos] + "/" + DRIVE_COMMAND_TYPE_);
  }
  for (size_t pos = 0; pos < PIVOTS_PER_SIDE_; ++pos)
  {
    conf_names.push_back(base_params_->left_pivot_names[pos] + "/" + PIVOT_COMMAND_TYPE_);
    conf_names.push_back(base_params_->right_pivot_names[pos] + "/" + PIVOT_COMMAND_TYPE_);
  }

  return {interface_configuration_type::INDIVIDUAL, conf_names};
}

InterfaceConfiguration NovaDriveControllerBase::state_interface_configuration() const
{
  if (base_params_->open_loop)
  {
    return {interface_configuration_type::NONE, {}};
  }

  std::vector<std::string> conf_names;
  for (size_t pos = 0; pos < wheels_per_side_; ++pos)
  {
    conf_names.push_back(base_params_->left_drive_names[pos] + "/" + drive_feedback_type());
    conf_names.push_back(base_params_->right_drive_names[pos] + "/" + drive_feedback_type());
  }
  for (size_t pos = 0; pos < PIVOTS_PER_SIDE_; ++pos)
  {
    conf_names.push_back(base_params_->left_pivot_names[pos] + "/" + pivot_feedback_type());
    conf_names.push_back(base_params_->right_pivot_names[pos] + "/" + pivot_feedback_type());
  }

  return {interface_configuration_type::INDIVIDUAL, conf_names};
}

controller_interface::return_type NovaDriveControllerBase::update(
  const rclcpp::Time& time, const rclcpp::Duration& period)
{
  auto logger = get_node()->get_logger();

  if (base_param_listener_->is_old(*base_params_))
  {
    *base_params_ = base_param_listener_->get_params();
    RCLCPP_INFO(logger, C_INFO "Parameters were updated" C_END);
  }
  update_params();

  std::shared_ptr<TwistStamped> command_msg_ptr = *(received_twist_msg_ptr_.readFromRT());
  if (command_msg_ptr == nullptr)
  {
    RCLCPP_WARN(logger, C_WARN "Received TwistStamped message was a nullptr." C_END);
    return controller_interface::return_type::ERROR;
  }
  else if ((std::isnan(command_msg_ptr->twist.linear.x) ||
            std::isnan(command_msg_ptr->twist.angular.z)))
  {
    RCLCPP_WARN_SKIPFIRST_THROTTLE(
      logger, *get_node()->get_clock(), cmd_vel_receive_timeout_.seconds() * 1000,
      C_WARN "Command message contains NaNs. Not updating reference interfaces." C_END);
    return controller_interface::return_type::OK;
  }

  // ####################### Process input ###############################
  Commands cmds;

  const auto age_of_last_command = time - last_received_time_;
  if (age_of_last_command > cmd_vel_command_timeout_)
  {
    cmds = twist_to_commands(Twist(), base_params_->autonomous_mode, period);

    if (age_of_last_command < connection_timeout_)
    {
      RCLCPP_WARN_THROTTLE(
        logger, *get_node()->get_clock(), 500,
        C_WARN "Cmd age %.2fs exceeds timeout %.2fs; publishing zero commands." C_END,
        age_of_last_command.seconds(), cmd_vel_command_timeout_.seconds());
    }
    else if (!disconnected_)
    {
      disconnected_ = true;
      RCLCPP_ERROR_SKIPFIRST(
        logger, C_ERROR "The last received command is %.2f seconds old, which exceeds the connection timeout of "
                "%.2f seconds. Connection to the command publisher is assumed to be lost, no further warnings "
                "will be issued." C_END,
        age_of_last_command.seconds(), connection_timeout_.seconds());
    }
  }
  else
  {
    if (disconnected_)
    {
      disconnected_ = false;
      RCLCPP_INFO_SKIPFIRST(
        logger,
        C_SUCCESS "Connection has been restored! (Received a new command after not receiving any "
                  "for > %.2f seconds)" C_END, connection_timeout_.seconds());
    }

    cmds = twist_to_commands(command_msg_ptr->twist, base_params_->autonomous_mode, period);
  }

  // ################### Update and publish odometry #####################
  if (base_params_->open_loop)
  {
    odometry_->update_open_loop(
      cmds.linear_velocity_x, cmds.linear_velocity_y, cmds.angular_velocity, time);
  }
  else
  {
    Feedback feedback;

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
          C_INFO "Failed to get feedback from hardware interfaces for left/right drive joints %zu, "
          "odometry cannot be updated. Ending current update loop early." C_END,
          pos);
        return controller_interface::return_type::OK;
      }
      const double left_feedback = left_feedback_op.value();
      const double right_feedback = right_feedback_op.value();

      if (std::isnan(left_feedback) || std::isnan(right_feedback))
      {
        RCLCPP_ERROR(
          logger,
           C_ERROR "Received NaN feedback for left/right drive joints %zu, odometry cannot be updated. "
          "Ending current update loop early." C_END,
          pos);
        return controller_interface::return_type::ERROR;
      }

      feedback.left_drive.push_back(left_feedback);
      feedback.right_drive.push_back(right_feedback);
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
          C_INFO "Failed to get feedback from hardware interfaces for left/right pivot joints %zu, "
          "odometry cannot be updated. Ending current update loop early." C_END,
          pos);
        return controller_interface::return_type::OK;
      }
      const double left_feedback = left_feedback_op.value();
      const double right_feedback = right_feedback_op.value();

      if (std::isnan(left_feedback) || std::isnan(right_feedback))
      {
        RCLCPP_ERROR(
          logger,
           C_ERROR "Received NaN feedback for left/right pivot joints %zu, odometry cannot be updated. "
          "Ending current update loop early." C_END,
          pos);
        return controller_interface::return_type::ERROR;
      }

      feedback.left_pivot.push_back(left_feedback);
      feedback.right_pivot.push_back(right_feedback);
    }

    odometry_->update(feedback, time);
  }
  odometry_->publish(time);

  // ######################### Send commands #############################
  // Set command values for drive
  for (size_t pos = 0; pos < wheels_per_side_; ++pos)
  {
    if (
      !hwif_wrapper_->set_value(
        cmds.left_drive_speeds[pos] / base_params_->wheel_radius,
        encoded_pos(pos, JointSide::LEFT, JointType::DRIVE)) ||
      !hwif_wrapper_->set_value(
        cmds.right_drive_speeds[pos] / base_params_->wheel_radius,
        encoded_pos(pos, JointSide::RIGHT, JointType::DRIVE)))
    {
      RCLCPP_ERROR(logger, C_ERROR "Failed to set drive command values for position %zu." C_END, pos);
      return controller_interface::return_type::ERROR;
    }
  }
  // Set command values for pivots
  for (size_t pos = 0; pos < PIVOTS_PER_SIDE_; ++pos)
  {
    if (
      !hwif_wrapper_->set_value(
        cmds.left_pivot_positions[pos], encoded_pos(pos, JointSide::LEFT, JointType::PIVOT)) ||
      !hwif_wrapper_->set_value(
        cmds.right_pivot_positions[pos], encoded_pos(pos, JointSide::RIGHT, JointType::PIVOT)))
    {
      RCLCPP_ERROR(logger, C_ERROR "Failed to set pivot command values for position %zu." C_END, pos);
      return controller_interface::return_type::ERROR;
    }
  }

  // Publish commanded velocities
  if (base_params_->publish_commanded_velocities && realtime_commanded_twist_publisher_->trylock())
  {
    auto& commanded_twist_command = realtime_commanded_twist_publisher_->msg_;
    commanded_twist_command.header.stamp = time;
    commanded_twist_command.twist.linear.x = cmds.linear_velocity_x;
    commanded_twist_command.twist.linear.y = cmds.linear_velocity_y;
    commanded_twist_command.twist.linear.z = 0.0;
    commanded_twist_command.twist.angular.x = 0.0;
    commanded_twist_command.twist.angular.y = 0.0;
    commanded_twist_command.twist.angular.z = cmds.angular_velocity;
    realtime_commanded_twist_publisher_->unlockAndPublish();
  }
  
  return controller_interface::return_type::OK;
}

controller_interface::CallbackReturn NovaDriveControllerBase::on_configure(
  const rclcpp_lifecycle::State&)
{
  auto logger = get_node()->get_logger();

  // update parameters if they have changed
  if (base_param_listener_->is_old(*base_params_))
  {
    *base_params_ = base_param_listener_->get_params();
    RCLCPP_INFO(logger,  C_INFO "Base parameters were updated" C_END);
  }
  update_params();

  if (base_params_->left_drive_names.size() != base_params_->right_drive_names.size())
  {
    RCLCPP_ERROR(
      logger, C_ERROR "The number of left wheels [%zu] and the number of right wheels [%zu] are different" C_END,
      base_params_->left_drive_names.size(), base_params_->right_drive_names.size());
    return controller_interface::CallbackReturn::ERROR;
  }

  if (base_params_->left_drive_names.empty())
  {
    RCLCPP_ERROR(logger, C_ERROR "Wheel names parameters are empty!" C_END);
    return controller_interface::CallbackReturn::ERROR;
  }

  if (base_params_->left_pivot_names.size() != 2 || base_params_->right_pivot_names.size() != 2)
  {
    RCLCPP_ERROR(
      logger, C_ERROR "Expected exactly two pivots per side, instead got %zu left and %zu right pivots" C_END,
      base_params_->left_pivot_names.size(), base_params_->right_pivot_names.size());
    return controller_interface::CallbackReturn::ERROR;
  }

  cmd_vel_receive_timeout_ = rclcpp::Duration::from_seconds(base_params_->cmd_vel_receive_timeout);
  cmd_vel_command_timeout_ = rclcpp::Duration::from_seconds(base_params_->cmd_vel_command_timeout);
  connection_timeout_ = rclcpp::Duration::from_seconds(base_params_->connection_timeout);

  limiter_drive_ = SpeedLimiter(
    base_params_->drive.has_velocity_limits, base_params_->drive.has_acceleration_limits,
    base_params_->drive.has_jerk_limits, base_params_->drive.min_velocity,
    base_params_->drive.max_velocity, base_params_->drive.min_acceleration,
    base_params_->drive.max_acceleration, base_params_->drive.min_jerk,
    base_params_->drive.max_jerk);
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
      DEFAULT_COMMAND_OUT_TOPIC_, rclcpp::SystemDefaultsQoS());
    realtime_commanded_twist_publisher_ =
      std::make_shared<realtime_tools::RealtimePublisher<TwistStamped>>(commanded_twist_publisher_);
  }

  // Initialise twist subscriber
  twist_subscriber_ = get_node()->create_subscription<TwistStamped>(
    DEFAULT_COMMAND_TOPIC_, rclcpp::QoS(1).best_effort(),
    [this, logger](const std::shared_ptr<TwistStamped> msg) -> void
    {
      if (!is_active_)
      {
        RCLCPP_WARN_ONCE(logger, C_WARN "Can't accept new commands, subscriber is inactive" C_END);
        return;
      }
      if ((msg->header.stamp.sec == 0) && (msg->header.stamp.nanosec == 0))
      {
        RCLCPP_WARN_ONCE(
          logger,
          C_WARN "Received TwistStamped with zero timestamp, setting it to current "
          "time, this message will only be shown once" C_END);
        msg->header.stamp = get_node()->get_clock()->now();
      }

      const auto current_time_diff = get_node()->now() - msg->header.stamp;

      if (
        cmd_vel_receive_timeout_ == rclcpp::Duration::from_seconds(0.0) ||
        current_time_diff < cmd_vel_receive_timeout_)
      {
        received_twist_msg_ptr_.writeFromNonRT(msg);
        last_received_time_ = get_node()->now();
      }
      else
      {
        RCLCPP_WARN(
          logger,
          C_WARN "Ignoring the received message (timestamp %.10f) because it is older than "
          "the current time by %.10f seconds, which exceeds the allowed timeout (%.4f)" C_END,
          rclcpp::Time(msg->header.stamp).seconds(), current_time_diff.seconds(),
          cmd_vel_receive_timeout_.seconds());
      }
    });

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaDriveControllerBase::on_activate(
  const rclcpp_lifecycle::State&)
{
  auto logger = get_node()->get_logger();

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
    RCLCPP_ERROR(logger,  C_ERROR "Error configuring drives and pivots" C_END);
    return controller_interface::CallbackReturn::ERROR;
  }

  is_active_ = true;

  RCLCPP_INFO(logger, C_SUCCESS "Subscriber and publisher are now active." C_END);
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaDriveControllerBase::on_deactivate(
  const rclcpp_lifecycle::State&)
{
  is_active_ = false;
  halt();
  reset_buffers();

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaDriveControllerBase::on_cleanup(
  const rclcpp_lifecycle::State&)
{
  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaDriveControllerBase::on_error(
  const rclcpp_lifecycle::State&)
{
  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaDriveControllerBase::on_shutdown(
  const rclcpp_lifecycle::State&)
{
  return controller_interface::CallbackReturn::SUCCESS;
}

bool NovaDriveControllerBase::reset()
{
  odometry_->reset();
  reset_buffers();
  twist_subscriber_.reset();
  is_active_ = false;

  return true;
}

void NovaDriveControllerBase::reset_buffers()
{
  hwif_wrapper_->reset_handles();
  reset_limiter_buffers();

  // Reset received twist message to zeroes
  received_twist_msg_ptr_.reset();
  std::shared_ptr<TwistStamped> default_twist_ptr = std::make_shared<TwistStamped>();
  default_twist_ptr->header.stamp = get_node()->now();
  received_twist_msg_ptr_.writeFromNonRT(default_twist_ptr);
}

void NovaDriveControllerBase::halt()
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

}  // namespace nova_drive_controller_base