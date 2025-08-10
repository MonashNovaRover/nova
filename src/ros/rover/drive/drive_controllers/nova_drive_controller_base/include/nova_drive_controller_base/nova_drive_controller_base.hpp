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

#ifndef NOVA_DRIVE_CONTROLLER_BASE__NOVA_DRIVE_CONTROLLER_BASE_HPP_
#define NOVA_DRIVE_CONTROLLER_BASE__NOVA_DRIVE_CONTROLLER_BASE_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <deque>
#include <string>
#include <vector>
#include <utility>
#include <algorithm>
#include <cstdio>
#include <limits>
#include <queue>
#include <ranges>
#include <tuple>

#include "controller_interface/controller_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "std_srvs/srv/empty.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"

#include "nova_controller_common/speed_limiter.hpp"
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

constexpr size_t encoded_pos(const size_t pos, const JointSide side, const JointType type)
{
  return pos << 2 | (static_cast<size_t>(side) << 1) | static_cast<size_t>(type);
}

struct Commands
{
  double linear_x_velocity;
  double linear_y_velocity;
  double angular_velocity;
  std::vector<double> left_wheel_speeds;      // left wheel speeds
  std::vector<double> right_wheel_speeds;     // right wheel speeds
  std::vector<double> left_pivot_positions;   // left pivot positions
  std::vector<double> right_pivot_positions;  // right pivot positions
};

using namespace std::chrono_literals;
using namespace nova_controller_common;
using controller_interface::interface_configuration_type;
using controller_interface::InterfaceConfiguration;
using geometry_msgs::msg::TwistStamped;
using hardware_interface::HW_IF_POSITION;
using hardware_interface::HW_IF_VELOCITY;
using lifecycle_msgs::msg::State;

template <typename Derived>
class NovaDriveControllerBase : public controller_interface::ControllerInterface
{
public:
  NovaDriveControllerBase()
    : controller_interface::ControllerInterface()
    , DRIVE_COMMAND_TYPE_(HW_IF_VELOCITY)
    , PIVOT_COMMAND_TYPE_(HW_IF_POSITION)
    , DEFAULT_COMMAND_TOPIC("/cmd_vel")
    , DEFAULT_COMMAND_OUT_TOPIC("~/cmd_vel_out")
    , PIVOTS_PER_SIDE_(2)  // two pivots per side: front and back
  {
  }

  controller_interface::CallbackReturn on_init() override
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
    hwif_wrapper_ = std::make_unique<HardwareInterfaceWrapper>(
      get_node(), state_interfaces_, command_interfaces_);

    // Initialise odometry
    odometry_ = std::make_unique<Odometry>(get_node(), base_params_);

    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::InterfaceConfiguration command_interface_configuration() const override
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
  };

  controller_interface::InterfaceConfiguration state_interface_configuration() const override
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

  virtual controller_interface::return_type update(
    const rclcpp::Time& time, const rclcpp::Duration& period) override
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
    Commands cmds = twist_to_commands(*command_msg_ptr, base_params_->autonomous_mode);

    // ################### Update and publish odometry #####################
    if (base_params_->open_loop)
    {
      odometry_->update_open_loop(cmds.linear_velocity, cmds.angular_velocity, time);
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

        feedback.left_drive_feedback.push_back(left_feedback);
        feedback.right_drive_feedback.push_back(right_feedback);
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

        feedback.left_pivot_feedback.push_back(left_feedback);
        feedback.right_pivot_feedback.push_back(right_feedback);
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
          cmds.left_wheel_speeds[pos] / base_params_->wheel_radius,
          encoded_pos(pos, JointSide::LEFT, JointType::DRIVE)) ||
        !hwif_wrapper_->set_value(
          cmds.right_wheel_speeds[pos] / base_params_->wheel_radius,
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
          cmds.left_pivot_positions[pos] / base_params_->wheel_radius,
          encoded_pos(pos, JointSide::LEFT, JointType::PIVOT)) ||
        !hwif_wrapper_->set_value(
          cmds.right_pivot_positions[pos] / base_params_->wheel_radius,
          encoded_pos(pos, JointSide::RIGHT, JointType::PIVOT)))
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
      logger,
      "------------------------------------------------------------------------------------");

    /**
     * Derived classes should override update() and update the previous command values for limiting,
     * e.g.
     * previous_linear_velocities_.pop_front();
     * previous_linear_velocities_.push_back(linear_velocity);
     */

    // Publish commanded velocities
    if (
      base_params_->publish_commanded_velocities && realtime_commanded_twist_publisher_->trylock())
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

  controller_interface::CallbackReturn on_configure(const rclcpp_lifecycle::State&) override
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
        logger,
        "The number of left wheels [%zu] and the number of right wheels [%zu] are different",
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
        std::make_shared<realtime_tools::RealtimePublisher<TwistStamped>>(
          commanded_twist_publisher_);
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

  controller_interface::CallbackReturn on_activate(const rclcpp_lifecycle::State&) override
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

  controller_interface::CallbackReturn on_deactivate(const rclcpp_lifecycle::State&) override
  {
    is_active_ = false;
    halt();
    reset_buffers();

    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn on_cleanup(const rclcpp_lifecycle::State&) override
  {
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn on_error(const rclcpp_lifecycle::State&) override
  {
    if (!reset())
    {
      return controller_interface::CallbackReturn::ERROR;
    }

    return controller_interface::CallbackReturn::SUCCESS;
  }

  controller_interface::CallbackReturn on_shutdown(const rclcpp_lifecycle::State&) override
  {
    return controller_interface::CallbackReturn::SUCCESS;
  }

protected:
  virtual Commands twist_to_commands(
    const geometry_msgs::msg::TwistStamped& twist_msg, bool autonomous_mode) const = 0;

  const char* drive_feedback_type() const
  {
    return base_params_->drive_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
  }
  const char* pivot_feedback_type() const
  {
    return base_params_->pivot_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
  }

  bool reset()
  {
    odometry_->reset();
    reset_buffers();
    twist_subscriber_.reset();
    is_active_ = false;

    return true;
  }

  virtual void reset_buffers()
  {
    hwif_wrapper_->reset_handles();

    /**
     * Derived classes may override this method to reset their own buffers, e.g.
     * previous_speeds_ = {0.0, 0.0};
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

  void halt()
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

  const char* DRIVE_COMMAND_TYPE_;
  const char* PIVOT_COMMAND_TYPE_;

  const std::string DEFAULT_COMMAND_TOPIC;
  const std::string DEFAULT_COMMAND_OUT_TOPIC;

  bool is_active_ = false;
  bool is_halted_ = false;

  // Parameters from ROS for nova_drive_controller_base
  std::shared_ptr<ParamListener> base_param_listener_;
  std::shared_ptr<Params> base_params_;

  size_t wheels_per_side_;
  const size_t PIVOTS_PER_SIDE_;

  std::unique_ptr<nova_controller_common::HardwareInterfaceWrapper> hwif_wrapper_;
  std::unique_ptr<Odometry> odometry_;

  /**
   * Derived clases will declare their own buffers, e.g.
   * std::deque<double> previous_linear_velocities_;
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

}  // namespace nova_drive_controller_base

#endif  // NOVA_DRIVE_CONTROLLER_BASE__NOVA_DRIVE_CONTROLLER_BASE_HPP_
