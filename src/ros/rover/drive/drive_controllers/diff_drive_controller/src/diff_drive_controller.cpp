// Copyright (c) 2025 Monash Nova Rover
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
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 * Monash Nova Rover Team
 * 
 * PACKAGE:   diff_drive_controller
 * AUTHORS:	  Terry Tian
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */

#include <string>
#include <vector>

#include "controller_interface/controller_interface.hpp"
#include "rclcpp/rclcpp.hpp"

#include "diff_drive_controller/diff_drive_controller.hpp"

namespace diff_drive_controller
{

using geometry_msgs::msg::Twist;
using nova_drive_controller_base::Commands;

DiffDriveController::DiffDriveController()
  : nova_drive_controller_base::NovaDriveControllerBase()
{
}

void DiffDriveController::init_params()
{
}

void DiffDriveController::update_params()
{
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
    speed = linear_velocity == 0 ? std::abs(angular_velocity * base_params_->steering_track / 2.0)
                                 : linear_velocity;
  }
  else
  {
    // Manual operation: left stick controls speed and right stick controls the turning radius
    // Process raw angular input through a curve to calculate the turning radius
    // Prioritise keeping turning radius over speed
    speed = linear_input * base_params_->drive.max_velocity;
    linear_velocity = speed;
    double turning_radius = turning_radius_from_angular_input(angular_input);

    if (turning_radius == INFINITY)
    {
      angular_velocity = 0.0;
    }
    else if (turning_radius == 0)
    {
      // calculated wheel speeds will equal 'speed'
      angular_velocity =
        std::copysign(2.0 * speed / base_params_->steering_track, speed * angular_input);
      linear_velocity = 0.0;
    }
    else
    {
      // Calculate the angular velocity based on the turning radius and speed
      angular_velocity = speed / turning_radius;
    }
  }

  // Limit the linear and angular velocities
  limiter_drive_.limit(speed, previous_speeds_[1], previous_speeds_[0], period.seconds());

  if (linear_velocity != 0)
  {
    linear_velocity = speed;
  };

  limiter_angular_.limit(
    angular_velocity, previous_angular_velocities_[1], previous_angular_velocities_[0],
    period.seconds());

  // Calculate commands
  const double left_speed =
    linear_velocity - (angular_velocity * base_params_->steering_track / 2.0);
  const double right_speed =
    linear_velocity + (angular_velocity * base_params_->steering_track / 2.0);

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
