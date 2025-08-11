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
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "realtime_tools/realtime_publisher.h"
#include "tf2/LinearMath/Quaternion.h"

#include "nova_drive_controller_base/odometry.hpp"

namespace
{

constexpr auto DEFAULT_ODOMETRY_TOPIC = "~/odom";
constexpr auto DEFAULT_TRANSFORM_TOPIC = "~/tf";
constexpr double EPSILON = 1e-6;

}  // namespace

namespace nova_drive_controller_base
{

Odometry::Odometry(rclcpp_lifecycle::LifecycleNode::SharedPtr node, std::shared_ptr<Params> params)
  : node_(std::move(node))
  , base_params_(std::move(params))
  , timestamp_(0.0)
  , half_wheel_base_(base_params_->wheel_base / 2.0)
  , half_steering_track_(base_params_->steering_track / 2.0)
  , wheels_per_side_(base_params_->left_drive_names.size())
  , PIVOTS_PER_SIDE_(2)  // two pivots per side: front and back
  , linear_x_accumulator_(base_params_->velocity_rolling_window_size)
  , linear_y_accumulator_(base_params_->velocity_rolling_window_size)
  , angular_accumulator_(base_params_->velocity_rolling_window_size)
  , publish_period_(rclcpp::Duration::from_seconds(1.0 / base_params_->publish_rate))
{
  // Initialize publishers
  realtime_odometry_publisher_ =
    std::make_shared<realtime_tools::RealtimePublisher<nav_msgs::msg::Odometry>>(
      node_->create_publisher<nav_msgs::msg::Odometry>(
        DEFAULT_ODOMETRY_TOPIC, rclcpp::SystemDefaultsQoS()));

  // Append the tf prefix if there is one
  std::string tf_prefix = "";
  if (base_params_->tf_frame_prefix_enable)
  {
    tf_prefix = base_params_->tf_frame_prefix != "" ? base_params_->tf_frame_prefix
                                                    : std::string(node_->get_namespace());
    tf_prefix = tf_prefix == "/" ? "" : tf_prefix + "/";
  }

  const auto odom_frame_id = tf_prefix + base_params_->odom_frame_id;
  const auto base_frame_id = tf_prefix + base_params_->base_frame_id;

  auto& odometry_message = realtime_odometry_publisher_->msg_;
  odometry_message.header.frame_id = odom_frame_id;
  odometry_message.child_frame_id = base_frame_id;

  constexpr size_t NUM_DIMENSIONS = 6;
  for (size_t index = 0; index < 6; ++index)
  {
    // 0, 7, 14, 21, 28, 35
    const size_t diagonal_index = NUM_DIMENSIONS * index + index;
    odometry_message.pose.covariance[diagonal_index] =
      base_params_->pose_covariance_diagonal[index];
    odometry_message.twist.covariance[diagonal_index] =
      base_params_->twist_covariance_diagonal[index];
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

  prev_feedback_.left_drive.resize(wheels_per_side_);
  prev_feedback_.right_drive.resize(wheels_per_side_);
  prev_feedback_.left_pivot.resize(PIVOTS_PER_SIDE_);
  prev_feedback_.right_pivot.resize(PIVOTS_PER_SIDE_);
}

bool Odometry::update(const Feedback& feedback, const rclcpp::Time& time)
{
  const double dt = time.seconds() - timestamp_.seconds();
  if (dt < 0.0001)
  {
    return false;  // interval too small to integrate with
  }

  // Assume wheels are equidistant along the wheel base and steering track
  std::vector<Wheel> wheels;

  for (size_t pos = 0; pos < wheels_per_side_; ++pos)
  {
    double left_speed, right_speed;
    const double wheel_x =
      half_wheel_base_ - (pos * base_params_->wheel_base / (wheels_per_side_ - 1));

    if (base_params_->drive_position_feedback)
    {
      // Convert position feedback to velocity
      left_speed =
        ((feedback.left_drive[pos] - prev_feedback_.left_drive[pos]) * base_params_->wheel_radius) /
        dt;
      right_speed = ((feedback.right_drive[pos] - prev_feedback_.right_drive[pos]) *
                     base_params_->wheel_radius) /
                    dt;

      prev_feedback_.left_drive[pos] = feedback.left_drive[pos];
      prev_feedback_.right_drive[pos] = feedback.right_drive[pos];
    }
    else
    {
      // Velocity feedback is the angular velocity of the wheels
      left_speed = feedback.left_drive[pos] * base_params_->wheel_radius;
      right_speed = feedback.right_drive[pos] * base_params_->wheel_radius;
    }

    wheels.push_back({left_speed, 0.0, wheel_x, half_steering_track_});
    wheels.push_back({right_speed, 0.0, wheel_x, -half_steering_track_});
  }

  // Assume pivots are at the ends of the wheel base
  for (size_t pos = 0; pos < PIVOTS_PER_SIDE_; ++pos)
  {
    double left_angle, right_angle;
    if (base_params_->pivot_position_feedback)
    {
      left_angle = feedback.left_pivot[pos];
      right_angle = feedback.right_pivot[pos];
    }
    else
    {
      // Convert pivot angular velocity to position
      left_angle = prev_feedback_.left_pivot[pos] + feedback.left_pivot[pos] * dt;
      right_angle = prev_feedback_.right_pivot[pos] + feedback.right_pivot[pos] * dt;

      prev_feedback_.left_pivot[pos] = left_angle;
      prev_feedback_.right_pivot[pos] = right_angle;
    }

    const size_t i = pos == 0 ? 0 : wheels.size() - 2;
    wheels[i].angle = left_angle;
    wheels[i + 1].angle = right_angle;
  }

  Eigen::Vector3d v = compute_velocities(wheels);
  const double vx = v[0];       // linear velocity in x direction [m/s]
  const double vy = v[1];       // linear velocity in y direction [m/s]
  const double angular = v[2];  // angular velocity [rad/s]

  // Integrate odometry:
  integrate_rk2(vx * dt, vy * dt, angular * dt);

  // Estimate speeds using a rolling mean to filter them out:
  linear_x_accumulator_.accumulate(vx);
  linear_y_accumulator_.accumulate(vy);
  angular_accumulator_.accumulate(angular);

  linear_x_ = linear_x_accumulator_.getRollingMean();
  linear_y_ = linear_y_accumulator_.getRollingMean();
  angular_ = angular_accumulator_.getRollingMean();

  timestamp_ = time;

  return true;
}

void Odometry::update_open_loop(double linear_x, double linear_y, double angular, const rclcpp::Time& time)
{
  /// Save last linear and angular velocity:
  linear_x_ = linear_x;
  linear_y_ = linear_y;
  angular_ = angular;

  /// Integrate odometry:
  const double dt = time.seconds() - timestamp_.seconds();
  timestamp_ = time;
  integrate_rk2(linear_x * dt, linear_y * dt, angular * dt);
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
      odometry_message.twist.twist.linear.x = linear_x_;
      odometry_message.twist.twist.angular.z = angular_;
      realtime_odometry_publisher_->unlockAndPublish();
    }

    if (base_params_->enable_odom_tf && realtime_odometry_transform_publisher_->trylock())
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

  linear_x_accumulator_ = RollingMeanAccumulator(base_params_->velocity_rolling_window_size);
  angular_accumulator_ = RollingMeanAccumulator(base_params_->velocity_rolling_window_size);
}

Eigen::Vector3d Odometry::compute_velocities(const std::vector<Wheel>& wheels) const
{
  const int N = static_cast<int>(wheels.size());
  if (N == 0) return Eigen::Vector3d::Zero();

  Eigen::MatrixXd A(2 * N, 3);
  Eigen::VectorXd b(2 * N);

  for (int i = 0; i < N; ++i)
  {
    const auto& w = wheels[i];
    // Wheel linear velocity vector in robot frame
    const double vix = w.speed * std::cos(w.angle);
    const double viy = w.speed * std::sin(w.angle);

    // Model: [vix] = [1 0 -y_i] [vx]
    //        [viy]   [0 1  x_i] [vy]
    A(2 * i, 0) = 1.0;
    A(2 * i, 1) = 0.0;
    A(2 * i, 2) = -w.y;
    A(2 * i + 1, 0) = 0.0;
    A(2 * i + 1, 1) = 1.0;
    A(2 * i + 1, 2) = w.x;

    b(2 * i) = vix;
    b(2 * i + 1) = viy;
  }

  // Solve least-squares (handles overdetermined data)
  Eigen::Vector3d x = A.colPivHouseholderQr().solve(b);
  return x;  // [vx, vy, angular]
}

void Odometry::integrate_rk2(double delta_linear_x, double delta_linear_y, double delta_angular)
{
  const double direction = heading_ + delta_angular * 0.5;

  const double cos_dir = std::cos(direction);
  const double sin_dir = std::sin(direction);

  x_ += delta_linear_x * cos_dir - delta_linear_y * sin_dir;
  y_ += delta_linear_x * sin_dir + delta_linear_y * cos_dir;

  heading_ += delta_angular;
}

}  // namespace nova_drive_controller_base
