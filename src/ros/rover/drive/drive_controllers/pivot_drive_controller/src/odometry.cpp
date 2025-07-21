#include "pivot_drive_controller/odometry.hpp"

namespace pivot_drive_controller
{
Odometry::Odometry(size_t velocity_rolling_window_size)
: timestamp_(0.0),
  x_(0.0),
  y_(0.0),
  heading_(0.0),
  linear_(0.0),
  angular_(0.0),
  wheel_base_(0.0),
  wheel_radius_(0.0),
  left_wheel_old_pos_(0.0),
  right_wheel_old_pos_(0.0),
  left_pivot_old_pos_(0.0),
  right_pivot_old_pos_(0.0),
  velocity_rolling_window_size_(velocity_rolling_window_size),
  linear_accumulator_(velocity_rolling_window_size),
  angular_accumulator_(velocity_rolling_window_size)
{
}

void Odometry::init(const rclcpp::Time & time)
{
  // Reset accumulators and timestamp:
  resetAccumulators();
  timestamp_ = time;
}

/*
bool Odometry::update(double left_pos, double right_pos, const rclcpp::Time & time)
{
  // We cannot estimate the speed with very small time intervals:
  const double dt = time.seconds() - timestamp_.seconds();
  if (dt < 0.0001)
  {
    return false;  // Interval too small to integrate with
  }

  // Get current wheel joint positions:
  const double left_front_wheel_cur_pos = left_pos * left_wheel_radius_;
  const double right_front_wheel_cur_pos = right_pos * right_wheel_radius_;
  const double left_back_wheel_cur_pos = left_pos * left_wheel_radius_;
  const double right_back_wheel_cur_pos = right_pos * right_wheel_radius_;

  // Estimate velocity of wheels using old and current position:
  const double left_wheel_est_vel = left_wheel_cur_pos - left_wheel_old_pos_;
  const double right_wheel_est_vel = right_wheel_cur_pos - right_wheel_old_pos_;

  // Update old position with current:
  left_wheel_old_pos_ = left_wheel_cur_pos;
  right_wheel_old_pos_ = right_wheel_cur_pos;

  updateFromVelocity(left_wheel_est_vel, right_wheel_est_vel, time);

  return true;
}
*/

bool Odometry::update_odometry(
  const double linear_velocity, const double angular, const double dt)
{
  /// Integrate odometry:
  Odometry::integrateExact(linear_velocity * dt, angular*dt);

  /// We cannot estimate the speed with very small time intervals:
  if (dt < 0.0001)
  {
    return false;  // Interval too small to integrate with
  }

  /// Estimate speeds using a rolling mean to filter them out:
  linear_accumulator_.accumulate(linear_velocity);
  angular_accumulator_.accumulate(angular);

  linear_ = linear_accumulator_.getRollingMean();
  angular_ = angular_accumulator_.getRollingMean();

  // RCLCPP_INFO_STREAM(rclcpp::get_logger("Odom"), "linear_: " << linear_);
  // RCLCPP_INFO_STREAM(rclcpp::get_logger("Odom"), "angular_: " << angular_);

  return true;
}

bool Odometry::update(const double &fl_speed, const double &fr_speed, const double &rl_speed, const double &rr_speed,
  const double front_steering, const double rear_steering, const double &dt)
{
    //#TODO: Move calculations in pivot_drive_controller.cpp to here
  return false;
}

bool Odometry::updateFromVelocity(double left_vel, double right_vel, const rclcpp::Time & time)
{
  const double dt = time.seconds() - timestamp_.seconds();

  // Compute linear and angular diff:
  const double linear = (left_vel + right_vel) * 0.5;
  // Now there is a bug about scout angular velocity
  const double angular = (right_vel - left_vel) / wheel_base_;

  // Integrate odometry:
  integrateExact(linear, angular);

  timestamp_ = time;

  // Estimate speeds using a rolling mean to filter them out:
  linear_accumulator_.accumulate(linear / dt);
  angular_accumulator_.accumulate(angular / dt);

  linear_ = linear_accumulator_.getRollingMean();
  angular_ = angular_accumulator_.getRollingMean();

  return true;
}

void Odometry::updateOpenLoop(double linear_vel, double angular_vel, const rclcpp::Time & time)
{
  /// Save last linear and angular velocity:
  linear_ = linear_vel;
  angular_ = angular_vel;

  /// Integrate odometry:
  const double dt = time.seconds() - timestamp_.seconds();
  
  timestamp_ = time;
  integrateExact(linear_vel * dt, angular_vel * dt);
}

void Odometry::resetOdometry()
{
  x_ = 0.0;
  y_ = 0.0;
  heading_ = 0.0;
}

void Odometry::setWheelParams(double steering_track, double wheel_radius,
                       double wheel_base, double wheel_steering_y_offset)
{
  wheel_base_ = wheel_base;
  wheel_radius_ = wheel_radius;
  steering_track_ = steering_track;
  wheel_steering_y_offset_ = wheel_steering_y_offset;
}

void Odometry::setVelocityRollingWindowSize(size_t velocity_rolling_window_size)
{
  velocity_rolling_window_size_ = velocity_rolling_window_size;

  resetAccumulators();
}

void Odometry::integrateRungeKutta2(double linear, double angular)
{
  const double direction = heading_ + angular * 0.5;

  /// Runge-Kutta 2nd order integration:
  x_ += linear * cos(direction);
  y_ += linear * sin(direction);

  heading_ += angular;
}

void Odometry::integrateExact(double linear, double angular)
{
  if (fabs(angular) < 1e-6)
  {
    integrateRungeKutta2(linear, angular);
  }
  else
  {
    /// Exact integration (should solve problems when angular is zero):
    const double heading_old = heading_;
    const double r = linear / angular;
    heading_ += angular;
    x_ += r * (sin(heading_) - sin(heading_old));
    y_ += -r * (cos(heading_) - cos(heading_old));
  }
}

void Odometry::resetAccumulators()
{
  linear_accumulator_ = RollingMeanAccumulator(velocity_rolling_window_size_);
  angular_accumulator_ = RollingMeanAccumulator(velocity_rolling_window_size_);
}

}  // namespace pivot_drive_controller
