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
using geometry_msgs::msg::Twist;
using hardware_interface::HW_IF_POSITION;
using hardware_interface::HW_IF_VELOCITY;
using lifecycle_msgs::msg::State;
using nova_drive_controller_base::Commands;

DiffDriveController::DiffDriveController()
  : nova_drive_controller_base::NovaDriveControllerBase()
{
}

void DiffDriveController::init_params()
{
  // Initialize parameters specific to the diff drive controller
  param_listener_ = std::make_shared<ParamListener>(get_node());
  params_ = param_listener_->get_params();

  wheel_separation_ = params_.wheel_separation_multiplier * base_params_->steering_track;
  left_wheel_radius_ = params_.left_wheel_radius_multiplier * base_params_->wheel_radius;
  right_wheel_radius_ = params_.right_wheel_radius_multiplier * base_params_->wheel_radius;
}

void DiffDriveController::update_params()
{
  if (param_listener_->is_old(params_))
  {
    params_ = param_listener_->get_params();
    RCLCPP_INFO(get_node()->get_logger(), "Parameters were updated");
  }
}

Commands DiffDriveController::twist_to_commands(
  const Twist& twist_msg, bool autonomous_mode, const rclcpp::Duration& period)
{
  auto logger = get_node()->get_logger();

  double linear_input = twist_msg.linear.x;
  double angular_input = twist_msg.angular.z;
  double linear_velocity, angular_velocity;
  double speed;

  if (autonomous_mode)
  {
    linear_velocity = linear_input;
    angular_velocity = angular_input;
    speed = linear_velocity == 0 ? std::abs(angular_velocity * wheel_separation_ / 2.0)
                                   : linear_velocity;
  }
  else
  {
    // Manual operation: left stick controls speed and right stick controls the turning radius
    // Process raw angular input through a curve to calculate the turning radius
    // Prioritise keeping turning radius over speed
    speed = linear_input * base_params_->speed.max_velocity;
    linear_velocity = speed;
    double turning_radius = turning_radius_from_angular_input(angular_input);

    if (turning_radius == INFINITY)
    {
      angular_velocity = 0.0;
    }
    else if (turning_radius == 0)
    {
      // calculated wheel speeds will equal 'speed'
      angular_velocity = std::copysign(2.0 * speed / wheel_separation_, speed * angular_input);
      linear_velocity = 0.0;
    }
    else
    {
      // Calculate the angular velocity based on the turning radius and speed
      angular_velocity = speed / turning_radius;
    }
  }

  // Limit the linear and angular velocities
  limiter_speed_.limit(speed, previous_speeds_[0], previous_speeds_[1], period.seconds());

  if (linear_velocity != 0)
  {
    linear_velocity = speed;
  };

  limiter_angular_.limit(
    angular_velocity, previous_angular_velocities_[0], previous_angular_velocities_[1],
    period.seconds());

  // Calculate commands
  const double left_speed = linear_velocity - (angular_velocity * wheel_separation_ / 2.0);
  const double right_speed = linear_velocity + (angular_velocity * wheel_separation_ / 2.0);

  RCLCPP_DEBUG(
    logger, "Set drive commands: left_speed = %.2f, right_speed = %.2f", left_speed, right_speed);
  RCLCPP_DEBUG(
    logger, "------------------------------------------------------------------------------------");

  // Update the previous command values for limiting
  previous_speeds_.pop_front();
  previous_speeds_.push_back(speed);
  previous_angular_velocities_.pop_front();
  previous_angular_velocities_.push_back(angular_velocity);

  return {
    linear_velocity_x : linear_velocity,
    linear_velocity_y : 0.0,
    angular_velocity : angular_velocity,
    left_drive_speeds : std::vector<double>(wheels_per_side_, left_speed),
    right_drive_speeds : std::vector<double>(wheels_per_side_, right_speed),
    left_pivot_positions : {0.0, 0.0},
    right_pivot_positions : {0.0, 0.0}
  };
}

void DiffDriveController::reset_limiter_buffers()
{
  previous_speeds_ = {0.0, 0.0};
  previous_angular_velocities_ = {0.0, 0.0};
}

}  // namespace diff_drive_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
  diff_drive_controller::DiffDriveController, controller_interface::ControllerInterface)
