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
using geometry_msgs::msg::TwistStamped;
using hardware_interface::HW_IF_POSITION;
using hardware_interface::HW_IF_VELOCITY;
using lifecycle_msgs::msg::State;
using drive_interfaces::srv::DriveStatus;
using drive_interfaces::msg::DriveCommand;

NovaDriveControllerBase::NovaDriveControllerBase()
  : controller_interface::ControllerInterface()
  , PIVOTS_PER_SIDE_(2)  // two pivots per side: front and back
  , DRIVE_COMMAND_TYPE_(HW_IF_VELOCITY)
  , PIVOT_COMMAND_TYPE_(HW_IF_POSITION)
  , DEFAULT_COMMAND_TOPIC_("/cmd_vel")
  , DEFAULT_COMMAND_OUT_TOPIC_("~/cmd_out")
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
    RCLCPP_INFO(logger, "Parameters were updated");
  }
  update_params();

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
  Commands cmds;

  const auto age_of_last_command = time - command_msg_ptr->header.stamp;
  if (age_of_last_command > cmd_vel_timeout_)
  {
    cmds.linear_velocity_x = 0.0;
    cmds.linear_velocity_y = 0.0;
    cmds.angular_velocity = 0.0;
    cmds.left_drive_speeds.assign(wheels_per_side_, 0.0);
    cmds.right_drive_speeds.assign(wheels_per_side_, 0.0);
    cmds.left_pivot_positions.assign(PIVOTS_PER_SIDE_, 0.0);
    cmds.right_pivot_positions.assign(PIVOTS_PER_SIDE_, 0.0);
  }
  else
  {
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
      RCLCPP_ERROR(logger, "Failed to set drive command values for position %zu.", pos);
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
      RCLCPP_ERROR(logger, "Failed to set pivot command values for position %zu.", pos);
      return controller_interface::return_type::ERROR;
    }
  }

  // Publish commands
  if (base_params_->publish_commands && realtime_command_publisher_->trylock())
  {
    auto& published_command = realtime_command_publisher_->msg_;
    published_command.header.stamp = time;
    published_command.twist.linear.x = cmds.linear_velocity_x;
    published_command.twist.linear.y = cmds.linear_velocity_y;
    published_command.twist.linear.z = 0;
    published_command.twist.angular.x = 0.0;
    published_command.twist.angular.y = 0.0;
    published_command.twist.angular.z = cmds.angular_velocity;

    auto copy_values {[&logger](const std::vector<double>& from, std::vector<double>& to)
    {
      if (from.size() != to.size())
      {
        RCLCPP_ERROR(logger, "mismatch between vector lengths; filling with NaNs instead");
        std::fill(to.begin(), to.end(), NAN);
      }
      else
      {
        std::copy(from.begin(), from.end(), to.begin());
      }
    }};

    copy_values(cmds.left_drive_speeds, published_command.left_drive_speeds);
    copy_values(cmds.right_drive_speeds, published_command.right_drive_speeds);
    copy_values(cmds.left_pivot_positions, published_command.left_pivot_positions);
    copy_values(cmds.right_pivot_positions, published_command.right_pivot_positions);

    realtime_command_publisher_->unlockAndPublish();
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
    RCLCPP_INFO(logger, "Base parameters were updated");
  }
  update_params();

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

  limiter_drive_ = SpeedLimiter(
    base_params_->drive.has_velocity_limits, base_params_->drive.has_acceleration_limits,
    base_params_->drive.has_jerk_limits, base_params_->drive.min_velocity,
    base_params_->drive.max_velocity, base_params_->drive.min_acceleration,
    base_params_->drive.max_acceleration, base_params_->drive.min_jerk,
    base_params_->drive.max_jerk);
  limiter_drive_velocity_ = VelocityLimiter(
    base_params_->drive.has_velocity_limits, base_params_->drive.has_acceleration_limits,
    base_params_->drive.has_jerk_limits, base_params_->drive.max_velocity,
    base_params_->drive.max_acceleration, base_params_->drive.max_jerk);
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
  if (base_params_->publish_commands)
  {
    command_publisher_ = get_node()->create_publisher<DriveCommand>(
      DEFAULT_COMMAND_OUT_TOPIC_, rclcpp::SystemDefaultsQoS());
    realtime_command_publisher_ =
      std::make_shared<realtime_tools::RealtimePublisher<DriveCommand>>(command_publisher_);

    auto& published_command = realtime_command_publisher_->msg_;
    published_command.left_drive_speeds.resize(wheels_per_side_);
    published_command.right_drive_speeds.resize(wheels_per_side_);
    published_command.left_pivot_positions.resize(PIVOTS_PER_SIDE_);
    published_command.right_pivot_positions.resize(PIVOTS_PER_SIDE_);
  }

  // Initialise twist subscriber
  twist_subscriber_ = get_node()->create_subscription<TwistStamped>(
    DEFAULT_COMMAND_TOPIC_, rclcpp::SystemDefaultsQoS(),
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

  set_drive_status_service_ = get_node()->create_service<DriveStatus>(
    "/drive_controller/set_drive_status",
    [this](const std::shared_ptr<DriveStatus::Request> request, const std::shared_ptr<DriveStatus::Response> response)
    {
      if (!is_active_)
      {
        RCLCPP_WARN_ONCE(
          get_node()->get_logger(), "Can't update drive status, service is inactive");
        return;
      }

      hold_position_ = request->hold_position;
      response->success = true;
    }, rclcpp::SystemDefaultsQoS());

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn NovaDriveControllerBase::on_activate(
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