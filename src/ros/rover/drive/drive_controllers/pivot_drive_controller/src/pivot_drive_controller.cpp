/**
 * ROS conventions dictate that left = positive and right = negative.
 * The received twist messages from teleop follow this convention.
 * However, our BLCMDs expect the opposite, i.e. left = negative and right = positive.
 * As such, all angles are calculated as per ROS conventions and are only
 * inverted right before being sent to the BLCMDs.
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
#include "tf2/LinearMath/Quaternion.h"

#include "nova_controller_common/blcmd_wrapper.hpp"
#include "pivot_drive_controller/pivot_drive_controller.hpp"

namespace
{
constexpr auto DEFAULT_INPUT_TOPIC = "/cmd_vel";
constexpr auto DEFAULT_ODOMETRY_TOPIC = "~/odom";
constexpr auto DEFAULT_TRANSFORM_TOPIC = "~/tf";
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
  return params_.drive_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
}

const char* PivotDriveController::pivot_feedback_type() const
{
  return params_.pivot_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
}

std::pair<double, double> PivotDriveController::get_pivot_angles_from_radius(double radius)
{
  double half_wheel_base = params_.wheel_base / 2;
  double half_steering_track = params_.steering_track / 2;

  if (radius == INFINITY)
  {
    return {0.0, 0.0};  // straight line, no pivot angles
  }
  return {
    atan((half_wheel_base) / (radius + (half_steering_track))),
    atan((half_wheel_base) / (radius - (half_steering_track)))};
}

controller_interface::CallbackReturn PivotDriveController::on_init()
{
  try
  {
    // Create the parameter listener and get the parameters
    param_listener_ = std::make_shared<ParamListener>(get_node());
    params_ = param_listener_->get_params();
  }
  catch (const std::exception& e)
  {
    fprintf(stderr, "Exception thrown during init stage with message: %s \n", e.what());
    return controller_interface::CallbackReturn::ERROR;
  }

  zero_radius_ =
    sqrt(std::pow(params_.wheel_base / 2, 2) + std::pow(params_.steering_track / 2, 2));
  RCLCPP_INFO_STREAM(get_node()->get_logger(), "zero_radius_: " << zero_radius_);

  // angle at which the wheels are initially offset
  double offset_angle = atan(params_.steering_track / params_.wheel_base);
  RCLCPP_INFO_STREAM(get_node()->get_logger(), "offset_angle: " << offset_angle);

  blcmd_wrapper_ = std::make_unique<BLCMDWrapper>(
    get_node(), offset_angle, state_interfaces_, command_interfaces_);

  return controller_interface::CallbackReturn::SUCCESS;
}

InterfaceConfiguration PivotDriveController::command_interface_configuration() const
{
  std::vector<std::string> conf_names;
  for (const std::string& joint_pos : params_.joints)
  {
    conf_names.push_back(
      params_.drive_names.joints_map.at(joint_pos).value + "/" + DRIVE_COMMAND_TYPE);

    conf_names.push_back(
      params_.pivot_names.joints_map.at(joint_pos).value + "/" + PIVOT_COMMAND_TYPE);
  }

  return {interface_configuration_type::INDIVIDUAL, conf_names};
}

InterfaceConfiguration PivotDriveController::state_interface_configuration() const
{
  if (params_.open_loop)
  {
    return {interface_configuration_type::NONE, {}};
  }

  std::vector<std::string> conf_names;
  for (const std::string& joint_pos : params_.joints)
  {
    conf_names.push_back(
      params_.drive_names.joints_map.at(joint_pos).value + "/" + drive_feedback_type());

    conf_names.push_back(
      params_.pivot_names.joints_map.at(joint_pos).value + "/" + pivot_feedback_type());
  }

  return {interface_configuration_type::INDIVIDUAL, conf_names};
}

controller_interface::return_type PivotDriveController::update(
  const rclcpp::Time& time, const rclcpp::Duration& period)
{
  auto logger = get_node()->get_logger();

  double target_speed, target_left_angle, target_right_angle;
  double speed, left_angle, right_angle;

  std::shared_ptr<TwistStamped> command_msg_ptr = *(received_twist_msg_ptr_.readFromRT());

  if (command_msg_ptr == nullptr)
  {
    RCLCPP_WARN(logger, "Received TwistStamped message was a nullptr.");
    return controller_interface::return_type::ERROR;
  }
  else if (!(std::isnan(command_msg_ptr->twist.linear.x) ||
             !std::isnan(command_msg_ptr->twist.angular.z)))
  {
    RCLCPP_WARN_SKIPFIRST_THROTTLE(
      logger, *get_node()->get_clock(), cmd_vel_timeout_.seconds() * 1000,
      "Command message contains NaNs. Not updating reference interfaces.");
  }

  // Twist values are scalar values from -1.0 to 1.0,
  // where 1.0 is the maximum speed or angular velocity
  target_speed = command_msg_ptr->twist.linear.x * params_.linear.max_velocity;

  if (params_.autonomous_mode)
  {
    // TODO
  }
  else
  {
    // Manual operation: right stick controls the pivot angle
    // Convert angular velocity to target radius through a curve
    // Visualise 1/x ± 1 using Desmos or any other graphing tool
    double angular_scalar = command_msg_ptr->twist.angular.z;
    double target_radius;
    if (angular_scalar == 0)
    {
      target_radius = INFINITY;  // straight line
    }
    else
    {
      target_radius = (1.0 / angular_scalar) - std::copysign(1.0, angular_scalar);
    }

    std::tie(target_left_angle, target_right_angle) = get_pivot_angles_from_radius(target_radius);

    const auto age_of_last_command = time - command_msg_ptr->header.stamp;
    // Brake if cmd_vel has timed out, override the stored command
    if (age_of_last_command > cmd_vel_timeout_)
    {
      target_speed = 0.0;
      target_left_angle = 0.0;
      target_right_angle = 0.0;
    }
  }

  speed = target_speed;
  left_angle = target_left_angle;
  right_angle = target_right_angle;
  // Limit speed and position
  limiter_linear_.limit(
    target_speed, previous_linear_velocities_[0], previous_linear_velocities_[1], period.seconds());

  // float radius;
  // int direction;

  // std::tie(radius, direction) = get_best_effort_radius_direction(target_radius,
  // target_direction);
  // // RCLCPP_INFO(get_node()->get_logger(), "best_effort radius of %f and direction of %f",
  // radius,
  // // direction);

  // left_angle = get_pivot_angle_from_radius(radius, true, direction);
  // right_angle = get_pivot_angle_from_radius(radius, false, direction);

  //        RCLCPP_INFO(get_node()->get_logger(), "left_angle command: %f", left_angle);
  //        RCLCPP_INFO(get_node()->get_logger(), "right_angle command: %f", right_angle);

  // registered_left_pivot_handles_.at(0).command.get().set_value(left_angle);
  // registered_left_pivot_handles_.at(1).command.get().set_value(-left_angle);
  // registered_right_pivot_handles_.at(0).command.get().set_value(-right_angle);
  // registered_right_pivot_handles_.at(1).command.get().set_value(right_angle);

  // // Update Odometry
  // if (params_.open_loop)
  // {
  //   // #TODO: Fix open loop odom
  //   float angular_command = (target_speed / radius) * direction * -1;
  //   // RCLCPP_INFO(logger, "time: %f", time);
  //   odometry_.updateOpenLoop(target_speed, angular_command, time);
  // }
  // else
  // {
  //   // #TODO Make odometry fault tolerant (or at least recognise faults)
  //   const double front_right_wheel_value =
  //     registered_right_drive_handles_.at(0).state.get().get_value() * params_.wheel_radius;
  //   const double rear_right_wheel_value =
  //     registered_right_drive_handles_.at(1).state.get().get_value() * params_.wheel_radius;
  //   const double front_left_wheel_value =
  //     registered_left_drive_handles_.at(0).state.get().get_value() * params_.wheel_radius;
  //   const double rear_left_wheel_value =
  //     registered_left_drive_handles_.at(1).state.get().get_value() * params_.wheel_radius;

  //   const double front_right_steer_position =
  //     registered_right_pivot_handles_.at(0).state.get().get_value();
  //   const double rear_right_steer_position =
  //     registered_right_pivot_handles_.at(1).state.get().get_value();
  //   const double front_left_steer_position =
  //     registered_left_pivot_handles_.at(0).state.get().get_value();
  //   const double rear_left_steer_position =
  //     registered_left_pivot_handles_.at(1).state.get().get_value();

  //   if (
  //     !std::isnan(front_right_wheel_value) && !std::isnan(front_left_wheel_value) &&
  //     !std::isnan(rear_right_wheel_value) && !std::isnan(rear_left_wheel_value) &&
  //     !std::isnan(front_right_steer_position) && !std::isnan(front_left_steer_position) &&
  //     !std::isnan(rear_right_steer_position) && !std::isnan(rear_left_steer_position))
  //   {
  //     if (params_.pivot_position_feedback)
  //     {
  //       RCLCPP_DEBUG_STREAM(
  //         get_node()->get_logger(),
  //         "frw: " << front_right_wheel_value << " flw: " << front_left_wheel_value
  //                 << " rrw: " << rear_right_wheel_value << " rlw: " << rear_left_wheel_value);
  //       RCLCPP_DEBUG_STREAM(
  //         get_node()->get_logger(),
  //         "frp: " << front_right_steer_position << " flp: " << front_left_steer_position
  //                 << " rrp: " << rear_right_steer_position << " rlp: " <<
  //                 rear_left_steer_position);

  //       // #TODO: move to the update function in odometry class
  //       bool flp_left = front_left_steer_position < offset_angle_;
  //       bool frp_left = front_right_steer_position > offset_angle_;
  //       bool rlp_left = rear_left_steer_position < offset_angle_;
  //       bool rrp_left = rear_right_steer_position > offset_angle_;

  //       double frp_radius =
  //         get_radius_from_angle(-front_right_steer_position, frp_left) * (flp_left ? -1 : 1);
  //       RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "frp_radius: " << frp_radius);
  //       double flp_radius =
  //         get_radius_from_angle(front_left_steer_position, flp_left) * (flp_left ? -1 : 1);
  //       RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "flp_radius: " << flp_radius);
  //       double rrp_radius =
  //         get_radius_from_angle(rear_right_steer_position, rrp_left) * (flp_left ? -1 : 1);
  //       RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "rrp_radius: " << rrp_radius);
  //       double rlp_radius =
  //         get_radius_from_angle(-rear_left_steer_position, rlp_left) * (flp_left ? -1 : 1);
  //       RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "rlp_radius: " << rlp_radius);
  //       double mean_radius = (frp_radius + flp_radius + rrp_radius + rlp_radius) / 4;

  //       if (
  //         (std::fabs(front_left_steer_position - offset_angle_) < 0.001) |
  //         (std::fabs(front_right_steer_position + offset_angle_) < 0.001) |
  //         (std::fabs(rear_left_steer_position + offset_angle_) < 0.001) |
  //         (std::fabs(rear_right_steer_position - offset_angle_) < 0.001))
  //       {
  //         mean_radius = INFINITY;
  //       }

  //       RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "mean_radius: " << mean_radius);

  //       if (std::fabs(mean_radius) < 0.1) mean_radius = 0;

  //       double left_ratio = 1;
  //       double right_ratio = 1;
  //       double max_ratio;

  //       if (mean_radius != 0 && mean_radius != INFINITY)
  //       {
  //         left_ratio = std::sqrt(
  //                        std::pow(params_.wheel_base / 2, 2) +
  //                        std::pow(mean_radius + (params_.steering_track / 2), 2.0)) /
  //                      std::abs(mean_radius);
  //         right_ratio = std::sqrt(
  //                         std::pow(params_.wheel_base / 2, 2.0) +
  //                         std::pow(mean_radius - (params_.steering_track / 2), 2.0)) /
  //                       std::abs(mean_radius);
  //       }

  //       // #TODO: More robust system for invalid radius detection
  //       if (!std::isnan(left_ratio) && !std::isnan(right_ratio))
  //       {
  //         RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "left_ratio: " << left_ratio);
  //         RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "right_ratio: " << right_ratio);
  //         RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "max_ratio: " << max_ratio);

  //         max_ratio = std::max(std::abs(left_ratio), std::abs(right_ratio));

  //         double mean_speed =
  //           (front_right_wheel_value / (right_ratio) + rear_right_wheel_value / (right_ratio) +
  //            front_left_wheel_value / (left_ratio) + rear_left_wheel_value / (left_ratio)) /
  //           4;

  //         // #TODO: Paramatize this
  //         if (std::fabs(mean_speed) < 0.01) mean_speed = 0;

  //         RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "mean_speed: " << mean_speed);

  //         double angular;

  //         // TODO: Fix this direction to be consistent with the frame.
  //         if (mean_speed == 0 || mean_radius == INFINITY)
  //         {
  //           angular = 0;
  //         }
  //         else if (mean_radius == 0)
  //         {
  //           angular = (mean_speed / zero_radius_) * (flp_left ? 1 : -1);
  //         }
  //         else
  //         {
  //           angular = mean_speed / -mean_radius;
  //         }

  //         double linear = mean_radius == 0 ? 0 : mean_speed;

  //         RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "linear: " << linear);
  //         RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "angular: " << angular);

  //         odometry_.update_odometry(linear, angular, period.seconds());
  //       }
  //     }
  //   }
  // }

  // tf2::Quaternion orientation;
  // orientation.setRPY(0.0, 0.0, odometry_.getHeading());
  // // RCLCPP_INFO(logger, "heading: %f", odometry_.getHeading());

  // bool should_publish = false;
  // try
  // {
  //   if (previous_publish_timestamp_ + publish_period_ < time)
  //   {
  //     previous_publish_timestamp_ += publish_period_;
  //     should_publish = true;
  //   }
  // }
  // catch (const std::runtime_error&)
  // {
  //   // Handle exceptions when the time source changes and initialize publish timestamp
  //   previous_publish_timestamp_ = time;
  //   should_publish = true;
  // }

  // if (should_publish)
  // {
  //   //            RCLCPP_INFO(logger, "should_publish");
  //   if (realtime_odometry_publisher_->trylock())
  //   {
  //     auto& odometry_message = realtime_odometry_publisher_->msg_;
  //     odometry_message.header.stamp = time;
  //     odometry_message.pose.pose.position.x = odometry_.getX();
  //     odometry_message.pose.pose.position.y = odometry_.getY();
  //     odometry_message.pose.pose.orientation.x = orientation.x();
  //     odometry_message.pose.pose.orientation.y = orientation.y();
  //     odometry_message.pose.pose.orientation.z = orientation.z();
  //     odometry_message.pose.pose.orientation.w = orientation.w();
  //     odometry_message.twist.twist.linear.x = odometry_.getLinear();
  //     odometry_message.twist.twist.angular.z = odometry_.getAngular();
  //     realtime_odometry_publisher_->unlockAndPublish();
  //   }

  //   if (params_.enable_odom_tf && realtime_odometry_transform_publisher_->trylock())
  //   {
  //     auto& transform = realtime_odometry_transform_publisher_->msg_.transforms.front();
  //     transform.header.stamp = time;
  //     transform.transform.translation.x = odometry_.getX();
  //     transform.transform.translation.y = odometry_.getY();
  //     transform.transform.rotation.x = orientation.x();
  //     transform.transform.rotation.y = orientation.y();
  //     transform.transform.rotation.z = orientation.z();
  //     transform.transform.rotation.w = orientation.w();
  //     realtime_odometry_transform_publisher_->unlockAndPublish();
  //   }
  // }

  // float left_ratio = 1, right_ratio = 1;

  // if (radius != 0 && radius != INFINITY)
  // {
  //   left_ratio = sqrt(
  //                  pow(params_.wheel_base / 2, 2.0) +
  //                  pow(radius * direction + (params_.steering_track / 2), 2.0)) /
  //                radius;
  //   right_ratio = sqrt(
  //                   pow(params_.wheel_base / 2, 2.0) +
  //                   pow(radius * direction - (params_.wheel_base / 2), 2.0)) /
  //                 radius;
  // }

  // double left_velocity = target_speed * left_ratio;
  // double right_velocity = target_speed * right_ratio;
  // double max_velocity = std::max(abs(left_velocity), abs(right_velocity));

  // RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "Left Velocity: " << left_velocity);
  // RCLCPP_DEBUG_STREAM(get_node()->get_logger(), "Right Velocity: " << right_velocity);

  // if (
  //   params_.has_velocity_limits &&
  //   (abs(left_velocity) > params_.max_velocity || abs(right_velocity) > params_.max_velocity))
  // {
  //   left_velocity = left_velocity / max_velocity * params_.max_velocity;
  //   right_velocity = right_velocity / max_velocity * params_.max_velocity;
  //   RCLCPP_WARN_STREAM(
  //     get_node()->get_logger(), "Velocity limit exceeded, scaling velocity down to "
  //                                 << left_velocity / left_ratio << " and "
  //                                 << right_velocity / right_ratio);
  // }

  // for (size_t index = 0; index < static_cast<size_t>(params_.wheels_per_side); ++index)
  // {
  //   registered_left_drive_handles_.at(index).command.get().set_value(
  //     left_velocity / params_.wheel_radius);
  //   registered_right_drive_handles_.at(index).command.get().set_value(
  //     right_velocity / params_.wheel_radius);
  // }

  return controller_interface::return_type::OK;
}

controller_interface::CallbackReturn PivotDriveController::on_configure(
  const rclcpp_lifecycle::State&)
{
  auto logger = get_node()->get_logger();

  // Update parameters if they have changed
  if (param_listener_->is_old(params_))
  {
    params_ = param_listener_->get_params();
    RCLCPP_INFO(logger, "Parameters were updated");
  }

  cmd_vel_timeout_ = rclcpp::Duration::from_seconds(params_.cmd_vel_timeout);

  limiter_linear_ = SpeedLimiter(
    params_.linear.has_velocity_limits, params_.linear.has_acceleration_limits,
    params_.linear.has_jerk_limits, params_.linear.min_velocity, params_.linear.max_velocity,
    params_.linear.min_acceleration, params_.linear.max_acceleration, params_.linear.min_jerk,
    params_.linear.max_jerk);
  limiter_angular_ = PositionLimiter(
    params_.angular.has_velocity_limits, params_.angular.has_acceleration_limits,
    params_.angular.has_jerk_limits, params_.angular.min_velocity, params_.angular.max_velocity,
    params_.angular.min_acceleration, params_.angular.max_acceleration, params_.angular.min_jerk,
    params_.angular.max_jerk);

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

  // Initialize odometry publisher and message
  odometry_publisher_ = get_node()->create_publisher<nav_msgs::msg::Odometry>(
    DEFAULT_ODOMETRY_TOPIC, rclcpp::SystemDefaultsQoS());
  realtime_odometry_publisher_ =
    std::make_shared<realtime_tools::RealtimePublisher<nav_msgs::msg::Odometry>>(
      odometry_publisher_);

  // Append the tf prefix if there is one
  std::string tf_prefix = "";
  if (params_.tf_frame_prefix_enable)
  {
    if (params_.tf_frame_prefix != "")
    {
      tf_prefix = params_.tf_frame_prefix;
    }
    else
    {
      tf_prefix = std::string(get_node()->get_namespace());
    }

    if (tf_prefix == "/")
    {
      tf_prefix = "";
    }
    else
    {
      tf_prefix = tf_prefix + "/";
    }
  }

  const auto odom_frame_id = tf_prefix + params_.odom_frame_id;
  const auto base_frame_id = tf_prefix + params_.base_frame_id;

  auto& odometry_message = realtime_odometry_publisher_->msg_;
  odometry_message.header.frame_id = odom_frame_id;
  odometry_message.child_frame_id = base_frame_id;

  // Limit the publication on the topics /odom and /tf
  publish_period_ = rclcpp::Duration::from_seconds(1.0 / params_.publish_rate);

  constexpr size_t NUM_DIMENSIONS = 6;
  for (size_t index = 0; index < 6; ++index)
  {
    // 0, 7, 14, 21, 28, 35
    const size_t diagonal_index = NUM_DIMENSIONS * index + index;
    odometry_message.pose.covariance[diagonal_index] = params_.pose_covariance_diagonal[index];
    odometry_message.twist.covariance[diagonal_index] = params_.twist_covariance_diagonal[index];
  }

  // Initialize transform publisher and message
  odometry_transform_publisher_ = get_node()->create_publisher<tf2_msgs::msg::TFMessage>(
    DEFAULT_TRANSFORM_TOPIC, rclcpp::SystemDefaultsQoS());
  realtime_odometry_transform_publisher_ =
    std::make_shared<realtime_tools::RealtimePublisher<tf2_msgs::msg::TFMessage>>(
      odometry_transform_publisher_);

  // Keeping track of odom and base_link transforms only
  auto& odometry_transform_message = realtime_odometry_transform_publisher_->msg_;
  odometry_transform_message.transforms.resize(1);
  odometry_transform_message.transforms.front().header.frame_id = odom_frame_id;
  odometry_transform_message.transforms.front().child_frame_id = base_frame_id;

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn PivotDriveController::on_activate(
  const rclcpp_lifecycle::State&)
{
  // Configure joints using BLCMDWrapper
  std::vector<Joint> joints;
  for (const std::string& joint_pos_str : params_.joints)
  {
    JointPosition joint_pos = to_joint_position(joint_pos_str);

    joints.emplace_back(
      params_.drive_names.joints_map.at(joint_pos_str).value, drive_feedback_type(),
      DRIVE_COMMAND_TYPE, joint_pos, JointType::DRIVE);

    joints.emplace_back(
      params_.pivot_names.joints_map.at(joint_pos_str).value, pivot_feedback_type(),
      PIVOT_COMMAND_TYPE, joint_pos, JointType::PIVOT);
  }

  if (!blcmd_wrapper_->configure_joint_handles(joints, params_.open_loop))
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
  odometry_.resetOdometry();
  reset_buffers();
  twist_subscriber_.reset();
  is_active_ = false;

  return true;
}

void PivotDriveController::reset_buffers()
{
  blcmd_wrapper_->reset_handles();

  previous_linear_velocities_ = {0.0, 0.0};
  previous_angular_positions_ = {0.0, 0.0, 0.0};

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
      blcmd_wrapper_->set_value(0.0, joint_pos, joint_type);
    }
  }
}

}  // namespace pivot_drive_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
  pivot_drive_controller::PivotDriveController, controller_interface::ControllerInterface)
