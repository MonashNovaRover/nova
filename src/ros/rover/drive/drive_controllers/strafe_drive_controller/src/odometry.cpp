#include <memory>
#include <utility>
#include <string>
#include <cmath>

#include "rclcpp/rclcpp.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "realtime_tools/realtime_publisher.h"
#include "tf2/LinearMath/Quaternion.h"

#include "strafe_drive_controller/odometry.hpp"

namespace
{

constexpr auto DEFAULT_ODOMETRY_TOPIC = "~/odom";
constexpr auto DEFAULT_TRANSFORM_TOPIC = "~/tf";
constexpr double EPSILON = 1e-6;

}  // namespace

namespace strafe_drive_controller
{

Odometry::Odometry(rclcpp_lifecycle::LifecycleNode::SharedPtr node, std::shared_ptr<Params> params)
  : node_(std::move(node))
  , params_(std::move(params))
  , timestamp_(0.0)
  , linear_accumulator_(params_->velocity_rolling_window_size)
  , publish_period_(rclcpp::Duration::from_seconds(1.0 / params_->publish_rate))
{
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

bool Odometry::update(double drive_feedback, const rclcpp::Time& time)
{
  // We cannot estimate the speed with very small time intervals:
  const double dt = time.seconds() - timestamp_.seconds();
  if (dt < 0.0001)
  {
    return false;  // interval too small to integrate with
  }

  double linear_velocity;

  if (params_->drive_position_feedback)
  {
    // Convert position feedback to velocity
    double drive_pos = drive_feedback * params_->wheel_radius;
    linear_velocity = (drive_pos - drive_old_pos_) / dt;
    drive_old_pos_ = drive_pos;
  }
  else
  {
    // Velocity feedback is the angular velocity of the wheels
    linear_velocity = drive_feedback * params_->wheel_radius;
  }

  // Integrate odometry:
  integrate_exact(linear_velocity * dt);

  timestamp_ = time;

  // Estimate speeds using a rolling mean to filter them out:
  linear_accumulator_.accumulate(linear_velocity);
  linear_ = linear_accumulator_.getRollingMean();

  return true;
}

void Odometry::update_open_loop(double linear, const rclcpp::Time& time)
{
  /// Save last linear velocity:
  linear_ = linear;

  /// Integrate odometry:
  const double dt = time.seconds() - timestamp_.seconds();
  timestamp_ = time;
  integrate_exact(linear * dt);
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
      odometry_message.twist.twist.linear.y = linear_;
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

  linear_accumulator_ = RollingMeanAccumulator(params_->velocity_rolling_window_size);
}

void Odometry::integrate_exact(double delta_linear)
{
  // Exact integration:
  y_ += delta_linear;
}

}  // namespace strafe_drive_controller
