#ifndef STRAFE_CONTROLLER__ODOMETRY_HPP_
#define STRAFE_CONTROLLER__ODOMETRY_HPP_

#include <cmath>
#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "rcpputils/rolling_mean_accumulator.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "realtime_tools/realtime_publisher.h"

#include "strafe_controller_parameters.hpp"

namespace strafe_controller
{

class Odometry
{
public:
  explicit Odometry(rclcpp_lifecycle::LifecycleNode::SharedPtr node, std::shared_ptr<Params> params);

  bool update(double drive_feedback, const rclcpp::Time& time);
  void update_open_loop(double linear, const rclcpp::Time& time);
  void publish(const rclcpp::Time& time);
  void reset();

private:
  using RollingMeanAccumulator = rcpputils::RollingMeanAccumulator<double>;

  void integrate_exact(double delta_linear);

  rclcpp_lifecycle::LifecycleNode::SharedPtr node_;
  std::shared_ptr<Params> params_;
  rclcpp::Time timestamp_;  // current timestamp

  // Current pose:
  double x_ = 0.0;        //   [m]
  double y_ = 0.0;        //   [m]
  double heading_ = 0.0;  // [rad]

  // Current velocity:
  double linear_ = 0.0;   //   [m/s]

  // Previous wheel position [rad]:
  double drive_old_pos_ = 0.0;

  // Rolling mean accumulator for linear velocity:
  RollingMeanAccumulator linear_accumulator_;

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

}  // namespace strafe_controller

#endif  // STRAFE_CONTROLLER__ODOMETRY_HPP_
