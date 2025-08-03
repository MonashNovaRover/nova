#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"
#include "tf2/LinearMath/Quaternion.h"

#include "diff_drive_controller/diff_drive_controller.hpp"

namespace
{

constexpr auto DEFAULT_COMMAND_TOPIC = "/cmd_vel";
constexpr auto DEFAULT_COMMAND_OUT_TOPIC = "~/cmd_vel_out";

}  // namespace

namespace diff_drive_controller
{

using namespace std::chrono_literals;
using namespace nova_controller_common;
using controller_interface::interface_configuration_type;
using controller_interface::InterfaceConfiguration;
using geometry_msgs::msg::TwistStamped;
using hardware_interface::HW_IF_POSITION;
using hardware_interface::HW_IF_VELOCITY;
using lifecycle_msgs::msg::State;

DiffDriveController::DiffDriveController()
  : controller_interface::ControllerInterface()
  , PIVOTS_PER_SIDE_(2)  // two pivots per side: front and back
  , DRIVE_COMMAND_TYPE_(HW_IF_VELOCITY)
  , PIVOT_COMMAND_TYPE_(HW_IF_POSITION)
{
}

const char* DiffDriveController::drive_feedback_type() const
{
  return params_->drive_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
}

const char* DiffDriveController::pivot_feedback_type() const
{
  return params_->pivot_position_feedback ? HW_IF_POSITION : HW_IF_VELOCITY;
}

controller_interface::CallbackReturn DiffDriveController::on_init()
{
  try
  {
    // Create the parameter listener and get the parameters
    param_listener_ = std::make_shared<ParamListener>(get_node());
    params_ = std::make_shared<Params>(param_listener_->get_params());
  }
  catch (const std::exception& e)
  {
    fprintf(stderr, "Exception thrown during init stage with message: %s \n", e.what());
    return controller_interface::CallbackReturn::ERROR;
  }

  wheel_separation_ = params_->wheel_separation_multiplier * params_->steering_track;
  left_wheel_radius_ = params_->left_wheel_radius_multiplier * params_->wheel_radius;
  right_wheel_radius_ = params_->right_wheel_radius_multiplier * params_->wheel_radius;
  wheels_per_side_ = params_->left_drive_names.size();

  // Initialise hardware interface wrapper
  hwif_wrapper_ = std::make_unique<HardwareInterfaceWrapper>(
    get_node(), params_->offset_angle, state_interfaces_, command_interfaces_);

  // Initialise odometry
  odometry_ = std::make_unique<Odometry>(get_node(), params_);

  return controller_interface::CallbackReturn::SUCCESS;
}

InterfaceConfiguration DiffDriveController::command_interface_configuration() const
{
  std::vector<std::string> conf_names;
  for (size_t i = 0; i < wheels_per_side_; ++i)
  {
    conf_names.push_back(params_->left_drive_names[i] + "/" + DRIVE_COMMAND_TYPE_);
    conf_names.push_back(params_->right_drive_names[i] + "/" + DRIVE_COMMAND_TYPE_);
  }
  for (size_t i = 0; i < PIVOTS_PER_SIDE_; ++i)
  {
    conf_names.push_back(params_->left_pivot_names[i] + "/" + PIVOT_COMMAND_TYPE_);
    conf_names.push_back(params_->right_pivot_names[i] + "/" + PIVOT_COMMAND_TYPE_);
  }

  return {interface_configuration_type::INDIVIDUAL, conf_names};
}

InterfaceConfiguration DiffDriveController::state_interface_configuration() const
{
  if (params_->open_loop)
  {
    return {interface_configuration_type::NONE, {}};
  }

  std::vector<std::string> conf_names;
  for (size_t i = 0; i < wheels_per_side_; ++i)
  {
    conf_names.push_back(params_->left_drive_names[i] + "/" + drive_feedback_type());
    conf_names.push_back(params_->right_drive_names[i] + "/" + drive_feedback_type());
  }
  for (size_t i = 0; i < PIVOTS_PER_SIDE_; ++i)
  {
    conf_names.push_back(params_->left_pivot_names[i] + "/" + pivot_feedback_type());
    conf_names.push_back(params_->right_pivot_names[i] + "/" + pivot_feedback_type());
  }

  return {interface_configuration_type::INDIVIDUAL, conf_names};
}

controller_interface::return_type DiffDriveController::update(
  const rclcpp::Time& time, const rclcpp::Duration& period)
{
  auto logger = get_node()->get_logger();

  if (param_listener_->is_old(*params_))
  {
    *params_ = param_listener_->get_params();
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
  // In manual operation, twist values are scalar values from -1.0 to 1.0,
  // where 1.0 is the maximum linear or angular velocity
  double linear_input = command_msg_ptr->twist.linear.x;
  double angular_input = command_msg_ptr->twist.angular.z;
  double linear_velocity, angular_velocity;

  // Brake if cmd_vel has timed out, override the stored command
  const auto age_of_last_command = time - command_msg_ptr->header.stamp;
  if (age_of_last_command > cmd_vel_timeout_)
  {
    linear_velocity = 0.0;
    angular_velocity = 0.0;
  }
  else if (params_->autonomous_mode)
  {
    linear_velocity = linear_input;
    angular_velocity = angular_input;
  }
  else
  {
    // Manual operation: left stick controls speed and right stick controls the pivot angle
    // Process raw angular input through a curve to calculate the turning radius
    // Prioritise keeping turning radius over speed
    linear_velocity = linear_input * params_->linear.max_velocity;
    const double turning_radius =
      angular_input == 0
        ? INFINITY
        : params_->input_curve_factor * ((1.0 / angular_input) - std::copysign(1, angular_input));
    angular_velocity = turning_radius == 0 ? 0.0 : linear_velocity / turning_radius;
  }

  // Limit the linear and angular velocities
  limiter_linear_.limit(
    linear_velocity, previous_linear_velocities_[0], previous_linear_velocities_[1],
    period.seconds());
  limiter_angular_.limit(
    angular_velocity, previous_angular_velocities_[0], previous_angular_velocities_[1],
    period.seconds());

  // ################### Update and publish odometry #####################
  if (params_->open_loop)
  {
    odometry_->update_open_loop(linear_velocity, angular_velocity, time);
  }
  else
  {
    double left_drive_feedback_mean = 0.0;
    double right_drive_feedback_mean = 0.0;

    // Drive feedback (average left and right drive joints)
    for (size_t pos = 0; pos < wheels_per_side_; ++pos)
    {
      const auto left_feedback_op =
        hwif_wrapper_->get_optional(pos, JointSide::LEFT, JointType::DRIVE);
      const auto right_feedback_op =
        hwif_wrapper_->get_optional(pos, JointSide::RIGHT, JointType::DRIVE);
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

      left_drive_feedback_mean += left_feedback;
      right_drive_feedback_mean += right_feedback;
    }

    left_drive_feedback_mean /= wheels_per_side_;
    right_drive_feedback_mean /= wheels_per_side_;

    odometry_->update(left_drive_feedback_mean, right_drive_feedback_mean, time);
  }
  odometry_->publish(time);

  // ######################### Send commands #############################
  const double left_velocity =
    (linear_velocity - angular_velocity * wheel_separation_ / 2.0) / left_wheel_radius_;
  const double right_velocity =
    (linear_velocity + angular_velocity * wheel_separation_ / 2.0) / right_wheel_radius_;

  // Set command values for drive
  for (size_t pos = 0; pos < wheels_per_side_; ++pos)
  {
    if (
      !hwif_wrapper_->set_value(left_velocity, pos, JointSide::LEFT, JointType::DRIVE) ||
      !hwif_wrapper_->set_value(right_velocity, pos, JointSide::RIGHT, JointType::DRIVE))
    {
      RCLCPP_ERROR(logger, "Failed to set drive command values for position %zu.", pos);
      return controller_interface::return_type::ERROR;
    }
  }
  // Set pivots to always face forward
  for (size_t pos = 0; pos < PIVOTS_PER_SIDE_; ++pos)
  {
    if (
      !hwif_wrapper_->set_value(0, pos, JointSide::LEFT, JointType::PIVOT) ||
      !hwif_wrapper_->set_value(0, pos, JointSide::RIGHT, JointType::PIVOT))
    {
      RCLCPP_ERROR(logger, "Failed to set pivot command values for position %zu.", pos);
      return controller_interface::return_type::ERROR;
    }
  }

  // Update the previous command values for limiting
  previous_linear_velocities_.pop_front();
  previous_linear_velocities_.push_back(linear_velocity);
  previous_angular_velocities_.pop_front();
  previous_angular_velocities_.push_back(angular_velocity);

  // Publish commanded velocities
  if (params_->publish_commanded_velocities && realtime_commanded_twist_publisher_->trylock())
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
  // float radius = angular_velocity;

  // if (radius == INFINITY || radius == -INFINITY || target_direction == 0)
  // {
  //   for (size_t index = 0; index < static_cast<size_t>(params_->wheels_per_side); ++index)
  //   {
  //     registered_left_drive_handles_[index].command.get().set_value(
  //       best_effort_velocity / params_->wheel_radius);
  //     registered_right_drive_handles_[index].command.get().set_value(
  //       best_effort_velocity / params_->wheel_radius);

  //     registered_left_pivot_handles_[index].command.get().set_value(
  //       angle_offset * (index == 0 ? 1 : -1));
  //     registered_right_pivot_handles_[index].command.get().set_value(
  //       angle_offset * (index == 0 ? -1 : 1));
  //   }
  // }
  // else
  // {
  //   // Calculate distances from the wheel_base centre to each wheel, and the maximum distance
  //   float left_wheel_distances[params_->wheels_per_side];
  //   float right_wheel_distances[params_->wheels_per_side];
  //   float wheel_x = params_->steering_track / 2;
  //   float wheel_y;

  //   float max_dist = 0;
  //   for (size_t index = 0; index < static_cast<size_t>(params_->wheels_per_side); ++index)
  //   {
  //     // position of wheel
  //     wheel_y = params_->wheel_base / 2 * (index == 0 ? 1 : -1);

  //     auto wheel_dist = [radius](float x, float y) -> float
  //     {
  //       return sqrt(pow(radius - x, 2) + pow(y, 2));
  //     };

  //     left_wheel_distances[index] = wheel_dist(-wheel_x, wheel_y);
  //     right_wheel_distances[index] = wheel_dist(wheel_x, wheel_y);

  //     max_dist = std::max({max_dist, left_wheel_distances[index], right_wheel_distances[index]});
  //   }
  //   // Handle sharp turning when radius is very small
  //   for (size_t index = 0; index < static_cast<size_t>(params_->wheels_per_side); ++index)
  //   {
  //     // Handle sharp turning when radius is very small
  //     bool sharp_turn = abs(radius) < (params_->steering_track) / 2;
  //     float left_speed = best_effort_velocity * left_wheel_distances[index] / max_dist;
  //     float right_speed = best_effort_velocity * right_wheel_distances[index] / max_dist;

  //     if (sharp_turn)
  //     {
  //       if (target_direction < 0)  // Left turn
  //       {
  //         left_speed = -left_speed;
  //       }
  //       else if (target_direction > 0)  // Right turn
  //       {
  //         right_speed = -right_speed;
  //       }
  //     }

  //     registered_left_drive_handles_[index].command.get().set_value(
  //       left_speed / params_->wheel_radius);
  //     registered_right_drive_handles_[index].command.get().set_value(
  //       right_speed / params_->wheel_radius);

  //     registered_left_pivot_handles_[index].command.get().set_value(
  //       angle_offset * (index == 0 ? 1 : -1));
  //     registered_right_pivot_handles_[index].command.get().set_value(
  //       angle_offset * (index == 0 ? -1 : 1));

  //     RCLCPP_INFO(
  //       logger, "Left wheel speed: %f, Right wheel speed: %f", left_speed /
  //       params_->wheel_radius, right_speed / params_->wheel_radius);
  //   }
  // }

  return controller_interface::return_type::OK;
}

controller_interface::CallbackReturn DiffDriveController::on_configure(
  const rclcpp_lifecycle::State&)
{
  auto logger = get_node()->get_logger();

  // update parameters if they have changed
  if (param_listener_->is_old(*params_))
  {
    *params_ = param_listener_->get_params();
    RCLCPP_INFO(logger, "Parameters were updated");
  }

  if (params_->left_drive_names.size() != params_->right_drive_names.size())
  {
    RCLCPP_ERROR(
      logger, "The number of left wheels [%zu] and the number of right wheels [%zu] are different",
      params_->left_drive_names.size(), params_->right_drive_names.size());
    return controller_interface::CallbackReturn::ERROR;
  }

  if (params_->left_drive_names.empty())
  {
    RCLCPP_ERROR(logger, "Wheel names parameters are empty!");
    return controller_interface::CallbackReturn::ERROR;
  }

  if (params_->left_pivot_names.size() != 2 || params_->right_pivot_names.size() != 2)
  {
    RCLCPP_ERROR(
      logger, "Expected exactly two pivots per side, instead got %zu left and %zu right pivots",
      params_->left_pivot_names.size(), params_->right_pivot_names.size());
    return controller_interface::CallbackReturn::ERROR;
  }

  cmd_vel_timeout_ = rclcpp::Duration::from_seconds(params_->cmd_vel_timeout);

  limiter_linear_ = SpeedLimiter(
    params_->linear.has_velocity_limits, params_->linear.has_acceleration_limits,
    params_->linear.has_jerk_limits, params_->linear.min_velocity, params_->linear.max_velocity,
    params_->linear.min_acceleration, params_->linear.max_acceleration, params_->linear.min_jerk,
    params_->linear.max_jerk);
  limiter_angular_ = SpeedLimiter(
    params_->angular.has_velocity_limits, params_->angular.has_acceleration_limits,
    params_->angular.has_jerk_limits, params_->angular.min_velocity, params_->angular.max_velocity,
    params_->angular.min_acceleration, params_->angular.max_acceleration, params_->angular.min_jerk,
    params_->angular.max_jerk);

  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }

  // Initialise twist publisher
  if (params_->publish_commanded_velocities)
  {
    commanded_twist_publisher_ = get_node()->create_publisher<TwistStamped>(
      DEFAULT_COMMAND_OUT_TOPIC, rclcpp::SystemDefaultsQoS());
    realtime_commanded_twist_publisher_ =
      std::make_shared<realtime_tools::RealtimePublisher<TwistStamped>>(commanded_twist_publisher_);
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

controller_interface::CallbackReturn DiffDriveController::on_activate(
  const rclcpp_lifecycle::State&)
{
  // Configure joints
  std::vector<Joint> joints;
  for (size_t pos = 0; pos < wheels_per_side_; ++pos)
  {
    std::string left_drive_name = params_->left_drive_names[pos];
    std::string right_drive_name = params_->right_drive_names[pos];
    joints.emplace_back(
      left_drive_name, drive_feedback_type(), DRIVE_COMMAND_TYPE_, pos, JointSide::LEFT,
      JointType::DRIVE);
    joints.emplace_back(
      right_drive_name, drive_feedback_type(), DRIVE_COMMAND_TYPE_, pos, JointSide::RIGHT,
      JointType::DRIVE);
  }
  for (size_t pos = 0; pos < PIVOTS_PER_SIDE_; ++pos)
  {
    std::string left_pivot_name = params_->left_pivot_names[pos];
    std::string right_pivot_name = params_->right_pivot_names[pos];
    joints.emplace_back(
      left_pivot_name, pivot_feedback_type(), PIVOT_COMMAND_TYPE_, pos, JointSide::LEFT,
      JointType::PIVOT);
    joints.emplace_back(
      right_pivot_name, pivot_feedback_type(), PIVOT_COMMAND_TYPE_, pos, JointSide::RIGHT,
      JointType::PIVOT);
  }

  if (!hwif_wrapper_->configure_joint_handles(joints, params_->open_loop))
  {
    RCLCPP_ERROR(get_node()->get_logger(), "Error configuring drives and pivots");
    return controller_interface::CallbackReturn::ERROR;
  }

  is_halted_ = false;
  is_active_ = true;

  RCLCPP_DEBUG(get_node()->get_logger(), "Subscriber and publisher are now active.");
  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn DiffDriveController::on_deactivate(
  const rclcpp_lifecycle::State&)
{
  is_active_ = false;
  halt();
  reset_buffers();

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn DiffDriveController::on_cleanup(const rclcpp_lifecycle::State&)
{
  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

controller_interface::CallbackReturn DiffDriveController::on_error(const rclcpp_lifecycle::State&)
{
  if (!reset())
  {
    return controller_interface::CallbackReturn::ERROR;
  }

  return controller_interface::CallbackReturn::SUCCESS;
}

bool DiffDriveController::reset()
{
  odometry_->reset();
  reset_buffers();
  twist_subscriber_.reset();
  is_active_ = false;

  return true;
}

void DiffDriveController::reset_buffers()
{
  hwif_wrapper_->reset_handles();

  previous_linear_velocities_ = {0.0, 0.0};
  previous_angular_velocities_ = {0.0, 0.0};

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

controller_interface::CallbackReturn DiffDriveController::on_shutdown(
  const rclcpp_lifecycle::State&)
{
  return controller_interface::CallbackReturn::SUCCESS;
}

void DiffDriveController::halt()
{
  // Send zero commands to all wheels and pivots
  for (size_t pos = 0; pos < wheels_per_side_; ++pos)
  {
    hwif_wrapper_->set_value(0.0, pos, JointSide::LEFT, JointType::DRIVE);
    hwif_wrapper_->set_value(0.0, pos, JointSide::RIGHT, JointType::DRIVE);
  }
  for (size_t pos = 0; pos < PIVOTS_PER_SIDE_; ++pos)
  {
    hwif_wrapper_->set_value(0.0, pos, JointSide::LEFT, JointType::PIVOT);
    hwif_wrapper_->set_value(0.0, pos, JointSide::RIGHT, JointType::PIVOT);
  }
}

}  // namespace diff_drive_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
  diff_drive_controller::DiffDriveController, controller_interface::ControllerInterface)
