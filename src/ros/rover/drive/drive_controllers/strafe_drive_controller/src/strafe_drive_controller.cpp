#include <cstdio>
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <utility>
#include <vector>
#include <tuple>

#include "hardware_interface/types/hardware_interface_type_values.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/logging.hpp"
#include "tf2/LinearMath/Quaternion.h"

#include "strafe_drive_controller/strafe_drive_controller.hpp"

namespace
{

constexpr auto DEFAULT_COMMAND_TOPIC = "/cmd_vel";
constexpr auto DEFAULT_COMMAND_OUT_TOPIC = "~/cmd_vel_out";

}  // namespace

namespace strafe_drive_controller
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

StrafeDriveController::StrafeDriveController()
  : nova_drive_controller_base::NovaDriveControllerBase()
{
}

void StrafeDriveController::init_params()
{
}

void StrafeDriveController::update_params()
{
}

Commands StrafeDriveController::twist_to_commands(
  const Twist& twist_msg, bool autonomous_mode, const rclcpp::Duration& period)
{
  auto logger = get_node()->get_logger();

  double linear_input = twist_msg.linear.y;  // lateral velocity
  double linear_velocity;

  if (autonomous_mode)
  {
    linear_velocity = linear_input;
  }
  else
  {
    linear_velocity = linear_input * base_params_->drive.max_velocity;
  }

  // Limit the linear velocity
  limiter_drive_.limit(linear_velocity, previous_speeds_[0], previous_speeds_[1], period.seconds());

  // Calculate comamnds
  std::vector<double> left_drive_speeds(wheels_per_side_);
  std::vector<double> right_drive_speeds(wheels_per_side_);
  for (size_t pos = 0; pos < wheels_per_side_; ++pos)
  {
    const int multiplier = (pos != wheels_per_side_ - 1) ? 1 : -1;
    left_drive_speeds[pos] = multiplier * linear_velocity;
    right_drive_speeds[pos] = -multiplier * linear_velocity;
  }
  // Set pivots to be parallel sideways
  // Angles are set at +- 90 degrees due to the offset angle
  std::vector<double> left_pivot_positions(PIVOTS_PER_SIDE_);
  std::vector<double> right_pivot_positions(PIVOTS_PER_SIDE_);
  for (size_t pos = 0; pos < PIVOTS_PER_SIDE_; ++pos)
  {
    const int multiplier = pos == 0 ? 1 : -1;
    left_pivot_positions[pos] = multiplier * M_PI_2;
    right_pivot_positions[pos] = -multiplier * M_PI_2;
  }

  // Update the previous command values for limiting
  previous_speeds_.pop_front();
  previous_speeds_.push_back(linear_velocity);

  return {
    .linear_velocity_x = 0.0,
    .linear_velocity_y = linear_velocity,
    .angular_velocity = 0.0,
    .left_drive_speeds = left_drive_speeds,
    .right_drive_speeds = right_drive_speeds,
    .left_pivot_positions = left_pivot_positions,
    .right_pivot_positions = right_pivot_positions,
  };
}

void StrafeDriveController::reset_limiter_buffers()
{
  previous_speeds_ = {0.0, 0.0};
}

}  // namespace strafe_drive_controller

#include "class_loader/register_macro.hpp"

CLASS_LOADER_REGISTER_CLASS(
  strafe_drive_controller::StrafeDriveController, controller_interface::ControllerInterface)
