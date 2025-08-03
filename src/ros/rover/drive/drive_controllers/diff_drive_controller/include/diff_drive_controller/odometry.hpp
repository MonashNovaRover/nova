#ifndef DIFF_DRIVE_CONTROLLER__ODOMETRY_HPP_
#define DIFF_DRIVE_CONTROLLER__ODOMETRY_HPP_

#include <cmath>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "rcpputils/rolling_mean_accumulator.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "realtime_tools/realtime_publisher.h"

#include "diff_drive_controller_parameters.hpp"

namespace diff_drive_controller
{

class Odometry
{
public:
  explicit Odometry(rclcpp_lifecycle::LifecycleNode::SharedPtr node, std::shared_ptr<Params> params);

  bool update(double left_drive_feedback, double right_drive_feedback, const rclcpp::Time& time);
  void update_open_loop(double linear, double angular, const rclcpp::Time& time);
  void publish(const rclcpp::Time& time);
  void reset();

private:
  using RollingMeanAccumulator = rcpputils::RollingMeanAccumulator<double>;

  void integrate_runge_kutta_2(double delta_linear, double delta_angular);
  void integrate_exact(double delta_linear, double delta_angular);

  rclcpp_lifecycle::LifecycleNode::SharedPtr node_;
  std::shared_ptr<Params> params_;
  rclcpp::Time timestamp_;  // current timestamp

  // Current pose:
  double x_ = 0.0;        //   [m]
  double y_ = 0.0;        //   [m]
  double heading_ = 0.0;  // [rad]

  // Current velocity:
  double linear_ = 0.0;   //   [m/s]
  double angular_ = 0.0;  // [rad/s]

  // Kinematic parameters [m]:
  double wheel_separation_;
  double left_wheel_radius_;
  double right_wheel_radius_;

  // Previous wheel positions [rad]:
  double left_drive_old_pos_ = 0.0;
  double right_drive_old_pos_ = 0.0;

  // Rolling mean accumulators for the linear and angular velocities:
  RollingMeanAccumulator linear_accumulator_;
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

}  // namespace diff_drive_controller

#endif  // DIFF_DRIVE_CONTROLLER__ODOMETRY_HPP_
