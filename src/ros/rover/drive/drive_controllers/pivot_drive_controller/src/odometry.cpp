// Copyright (c) 2020 PAL Robotics S.L.
//               2025 Monash Nova Rover
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
 * @brief Odometry class for the pivot drive controller.
 * Based off of Enrique Fernández from PAL Robotics' odometry implementation for
 * diff_drive_controller.
 *
 * @authors Terry Tian
 */

#include <memory>
#include <utility>
#include <string>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "realtime_tools/realtime_publisher.h"
#include "tf2/LinearMath/Quaternion.h"

#include "pivot_drive_controller/kinematics.hpp"
#include "pivot_drive_controller/odometry.hpp"

namespace
{

constexpr auto DEFAULT_ODOMETRY_TOPIC = "~/odom";
constexpr auto DEFAULT_TRANSFORM_TOPIC = "~/tf";
constexpr double EPSILON = 1e-6;

}  // namespace

namespace pivot_drive_controller
{

Odometry::Odometry(rclcpp_lifecycle::LifecycleNode::SharedPtr node, std::shared_ptr<Params> params)
  : node_(std::move(node))
  , params_(std::move(params))
  , timestamp_(0.0)
  , linear_accumulator_(params_->velocity_rolling_window_size)
  , angular_accumulator_(params_->velocity_rolling_window_size)
  , publish_period_(rclcpp::Duration::from_seconds(1.0 / params_->publish_rate))
{
  half_steering_track_ = params_->steering_track / 2;
  half_wheel_base_ = params_->wheel_base / 2;
  zero_radius_ = std::hypot(half_wheel_base_, half_steering_track_);
  // Solve for r = sqrt((r - half_steering_track_)^2 + half_wheel_base_^2)
  inner_radius_ = (std::pow(half_steering_track_, 2) + std::pow(half_wheel_base_, 2)) /
                  (2 * half_steering_track_);

  // Initialize publishers
  realtime_odometry_publisher_ =
    std::make_shared<realtime_tools::RealtimePublisher<nav_msgs::msg::Odometry>>(
      node_->create_publisher<nav_msgs::msg::Odometry>(
        DEFAULT_ODOMETRY_TOPIC, rclcpp::SystemDefaultsQoS()));

  // Append the tf prefix if there is one
  std::string tf_prefix = "";
  if (params_->tf_frame_prefix_enable)
  {
    tf_prefix = params_->tf_frame_prefix != "" ? params_->tf_frame_prefix
                                               : std::string(node_->get_namespace());
    tf_prefix = tf_prefix == "/" ? "" : tf_prefix + "/";
  }

  const auto odom_frame_id = tf_prefix + params_->odom_frame_id;
  const auto base_frame_id = tf_prefix + params_->base_frame_id;

  auto& odometry_message = realtime_odometry_publisher_->msg_;
  odometry_message.header.frame_id = odom_frame_id;
  odometry_message.child_frame_id = base_frame_id;

  constexpr size_t NUM_DIMENSIONS = 6;
  for (size_t index = 0; index < 6; ++index)
  {
    // 0, 7, 14, 21, 28, 35
    const size_t diagonal_index = NUM_DIMENSIONS * index + index;
    odometry_message.pose.covariance[diagonal_index] = params_->pose_covariance_diagonal[index];
    odometry_message.twist.covariance[diagonal_index] = params_->twist_covariance_diagonal[index];
  }

  // Initialize transform publisher and message
  odometry_transform_publisher_ = node_->create_publisher<tf2_msgs::msg::TFMessage>(
    DEFAULT_TRANSFORM_TOPIC, rclcpp::SystemDefaultsQoS());
  realtime_odometry_transform_publisher_ =
    std::make_shared<realtime_tools::RealtimePublisher<tf2_msgs::msg::TFMessage>>(
      odometry_transform_publisher_);

  // Keep track of odom and base_link transforms only
  auto& odometry_transform_message = realtime_odometry_transform_publisher_->msg_;
  odometry_transform_message.transforms.resize(1);
  odometry_transform_message.transforms.front().header.frame_id = odom_frame_id;
  odometry_transform_message.transforms.front().child_frame_id = base_frame_id;
}

bool Odometry::update(
  double left_drive_feedback, double right_drive_feedback, double left_pivot_feedback,
  double right_pivot_feedback, const rclcpp::Time& time)
{
  const double dt = time.seconds() - timestamp_.seconds();
  if (dt < 0.0001)
  {
    return false;  // Interval too small to integrate with
  }

  double left_speed, right_speed;
  double left_angle, right_angle;
  double linear_velocity, angular_velocity;

  if (params_->drive_position_feedback)
  {
    // Convert position feedback to velocity
    double left_drive_pos = left_drive_feedback * params_->wheel_radius;
    double right_drive_pos = right_drive_feedback * params_->wheel_radius;
    left_speed = (left_drive_pos - left_drive_old_pos_) / dt;
    right_speed = (right_drive_pos - right_drive_old_pos_) / dt;
    left_drive_old_pos_ = left_drive_pos;
    right_drive_old_pos_ = right_drive_pos;
  }
  else
  {
    // Velocity feedback is the angular velocity of the wheels
    left_speed = left_drive_feedback * params_->wheel_radius;
    right_speed = right_drive_feedback * params_->wheel_radius;
  }

  if (params_->pivot_position_feedback)
  {
    left_angle = left_pivot_feedback;
    right_angle = right_pivot_feedback;
  }
  else
  {
    // Convert pivot angular velocity to position
    left_angle = left_pivot_old_pos_ + left_pivot_feedback * dt;
    right_angle = right_pivot_old_pos_ + right_pivot_feedback * dt;
    left_pivot_old_pos_ = left_angle;
    right_pivot_old_pos_ = right_angle;
  }

  double turning_radius = get_radius_from_pivot_angle(
    left_angle, true, left_angle > 0, half_steering_track_, half_wheel_base_, EPSILON);
  double speed = (left_speed + right_speed) * 0.5;
  linear_velocity = turning_radius == 0 ? 0 : speed;
  angular_velocity = get_angular_from_radius_and_speed(
    turning_radius, speed, left_angle > 0, zero_radius_, inner_radius_);

  // Integrate odometry:
  integrate_exact(linear_velocity * dt, angular_velocity * dt);

  timestamp_ = time;

  // Estimate speeds using a rolling mean to filter them out:
  linear_accumulator_.accumulate(linear_velocity);
  angular_accumulator_.accumulate(angular_velocity);

  linear_ = linear_accumulator_.getRollingMean();
  angular_ = angular_accumulator_.getRollingMean();

  return true;
}

void Odometry::update_open_loop(double linear_vel, double angular_vel, const rclcpp::Time& time)
{
  /// Save last linear and angular velocity:
  linear_ = linear_vel;
  angular_ = angular_vel;

  /// Integrate odometry:
  const double dt = time.seconds() - timestamp_.seconds();
  timestamp_ = time;
  integrate_exact(linear_vel * dt, angular_vel * dt);
}

void Odometry::publish(const rclcpp::Time& time)
{
  tf2::Quaternion orientation;
  orientation.setRPY(0.0, 0.0, heading_);

  bool should_publish = false;
  try
  {
    if (previous_publish_timestamp_ + publish_period_ < time)
    {
      previous_publish_timestamp_ += publish_period_;
      should_publish = true;
    }
  }
  catch (const std::runtime_error&)
  {
    // Handle exceptions when the time source changes and initialize publish timestamp
    previous_publish_timestamp_ = time;
    should_publish = true;
  }

  if (should_publish)
  {
    if (realtime_odometry_publisher_->trylock())
    {
      auto& odometry_message = realtime_odometry_publisher_->msg_;
      odometry_message.header.stamp = time;
      odometry_message.pose.pose.position.x = x_;
      odometry_message.pose.pose.position.y = y_;
      odometry_message.pose.pose.orientation.x = orientation.x();
      odometry_message.pose.pose.orientation.y = orientation.y();
      odometry_message.pose.pose.orientation.z = orientation.z();
      odometry_message.pose.pose.orientation.w = orientation.w();
      odometry_message.twist.twist.linear.x = linear_;
      odometry_message.twist.twist.angular.z = angular_;
      realtime_odometry_publisher_->unlockAndPublish();
    }

    if (params_->enable_odom_tf && realtime_odometry_transform_publisher_->trylock())
    {
      auto& transform = realtime_odometry_transform_publisher_->msg_.transforms.front();
      transform.header.stamp = time;
      transform.transform.translation.x = x_;
      transform.transform.translation.y = y_;
      transform.transform.rotation.x = orientation.x();
      transform.transform.rotation.y = orientation.y();
      transform.transform.rotation.z = orientation.z();
      transform.transform.rotation.w = orientation.w();
      realtime_odometry_transform_publisher_->unlockAndPublish();
    }
  }
}

void Odometry::reset()
{
  x_ = 0.0;
  y_ = 0.0;
  heading_ = 0.0;
}

void Odometry::integrate_runge_kutta_2(double delta_linear, double delta_angular)
{
  const double direction = heading_ + delta_angular * 0.5;

  /// Runge-Kutta 2nd order integration:
  x_ += delta_linear * std::cos(direction);
  y_ += delta_linear * std::sin(direction);

  heading_ += delta_angular;
}

void Odometry::integrate_exact(double delta_linear, double delta_angular)
{
  if (std::abs(delta_angular) < EPSILON)
  {
    integrate_runge_kutta_2(delta_linear, delta_angular);
  }
  else
  {
    /// Exact integration (should solve problems when delta_angular is zero):
    const double heading_old = heading_;
    const double r = delta_linear / delta_angular;
    heading_ += delta_angular;
    x_ += r * (std::sin(heading_) - std::sin(heading_old));
    y_ += -r * (std::cos(heading_) - std::cos(heading_old));
  }
}

}  // namespace pivot_drive_controller
