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

#ifndef DRIVE_CONTROLLER_BASE__ODOMETRY_HPP_
#define DRIVE_CONTROLLER_BASE__ODOMETRY_HPP_

#include <cmath>
#include <memory>
#include <vector>

#include <Eigen/Dense>

#include "rclcpp/rclcpp.hpp"
#include "rcpputils/rolling_mean_accumulator.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "realtime_tools/realtime_publisher.h"

#include "drive_controller_base_parameters.hpp"

namespace drive_controller_base
{

struct Feedback
{
  std::vector<double> left_drive;
  std::vector<double> right_drive;
  std::vector<double> left_pivot;
  std::vector<double> right_pivot;
};

struct Wheel
{
  double speed;  // m/s (wheel angular velocity * wheel_radius)
  double angle;  // radians, 0 = forward (+x)
  double x;      // m, wheel position in robot frame
  double y;      // m, wheel position in robot frame
};

class Odometry
{
public:
  Odometry(rclcpp_lifecycle::LifecycleNode::SharedPtr node, std::shared_ptr<Params> params);

  bool update(const Feedback& feedback, const rclcpp::Time& time);
  void update_open_loop(double linear_x, double linear_y, double angular, const rclcpp::Time& time);
  void publish(const rclcpp::Time& time);
  void reset();

private:
  using RollingMeanAccumulator = rcpputils::RollingMeanAccumulator<double>;

  Eigen::Vector3d compute_velocities(const std::vector<Wheel>& wheels) const;
  void integrate_rk2(double delta_linear_x, double delta_linear_y, double delta_angular);

  rclcpp_lifecycle::LifecycleNode::SharedPtr node_;
  std::shared_ptr<Params> base_params_;
  rclcpp::Time timestamp_;  // current timestamp

  // Current pose:
  double x_ = 0.0;        //   [m]
  double y_ = 0.0;        //   [m]
  double heading_ = 0.0;  // [rad]

  // Current velocity:
  double linear_x_ = 0.0;  //   [m/s]
  double linear_y_ = 0.0;  //   [m/s]
  double angular_ = 0.0;   // [rad/s]

  // Kinematic parameters:
  double half_wheel_base_;      // [m]
  double half_steering_track_;  // [m]

  size_t wheels_per_side_;
  const size_t PIVOTS_PER_SIDE_;

  // Previous wheel and pivot positions [rad]:
  Feedback prev_feedback_;

  // Rolling mean accumulators for the linear and angular velocities:
  RollingMeanAccumulator linear_x_accumulator_;
  RollingMeanAccumulator linear_y_accumulator_;
  RollingMeanAccumulator angular_accumulator_;

  // Realtime publishers for odometry and transforms
  std::shared_ptr<realtime_tools::RealtimePublisher<nav_msgs::msg::Odometry>>
    realtime_odometry_publisher_;

  std::shared_ptr<rclcpp::Publisher<tf2_msgs::msg::TFMessage>> odometry_transform_publisher_;
  std::shared_ptr<realtime_tools::RealtimePublisher<tf2_msgs::msg::TFMessage>>
    realtime_odometry_transform_publisher_;

  // Publish rate limiter
  rclcpp::Duration publish_period_;
  rclcpp::Time previous_publish_timestamp_{0, 0, RCL_CLOCK_UNINITIALIZED};
};

}  // namespace drive_controller_base

#endif  // DRIVE_CONTROLLER_BASE__ODOMETRY_HPP_
