#include "strafe_controller/odometry.hpp"

namespace strafe_controller
{

Odometry::Odometry(size_t velocity_rolling_window_size)
  : timestamp_(0.0)
  , x_(0.0)
  , y_(0.0)
  , heading_(0.0)
  , linear_(0.0)
  , angular_(0.0)
  , wheel_base_(0.0)
  , wheel_radius_(0.0)
  , wheel_old_pos_(0.0)
  , velocity_rolling_window_size_(velocity_rolling_window_size)
  , linear_accumulator_(velocity_rolling_window_size)
  , angular_accumulator_(velocity_rolling_window_size)
{
}

void Odometry::init(const rclcpp::Time& time)
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

bool Odometry::update_odometry(const double linear_velocity, const double angular, const double dt)
{
  /// Integrate odometry:
  Odometry::integrateExact(linear_velocity * dt, angular);

  /// We cannot estimate the speed with very small time intervals:
  if (dt < 0.0001)
  {
    return false;  // Interval too small to integrate with
  }

  /// Estimate speeds using a rolling mean to filter them out:
  linear_accumulator_.accumulate(linear_velocity);
  angular_accumulator_.accumulate(angular / dt);

  linear_ = linear_accumulator_.getRollingMean();
  angular_ = angular_accumulator_.getRollingMean();

  return true;
}

bool Odometry::update(
  const double& fl_speed, const double& fr_speed, const double& rl_speed, const double& rr_speed,
  const double front_steering, const double rear_steering, const double& dt)
{
  const double front_tmp =
    cos(front_steering) * (tan(front_steering) - tan(rear_steering)) / wheel_base_;

  const double front_left_tmp =
    front_tmp / sqrt(
                  1 - steering_track_ * front_tmp * cos(front_steering) +
                  pow(steering_track_ * front_tmp / 2, 2));
  const double front_right_tmp =
    front_tmp / sqrt(
                  1 + steering_track_ * front_tmp * cos(front_steering) +
                  pow(steering_track_ * front_tmp / 2, 2));

  const double fl_speed_tmp = fl_speed * (1 / (1 - wheel_steering_y_offset_ * front_left_tmp));
  const double fr_speed_tmp = fr_speed * (1 / (1 - wheel_steering_y_offset_ * front_right_tmp));

  const double front_linear_speed =
    wheel_radius_ * copysign(1.0, fl_speed_tmp + fr_speed_tmp) *
    sqrt((pow(fl_speed, 2) + pow(fr_speed, 2)) / (2 + pow(steering_track_ * front_tmp, 2) / 2.0));

  const double rear_tmp =
    cos(rear_steering) * (tan(front_steering) - tan(rear_steering)) / wheel_base_;

  const double rear_left_tmp = rear_tmp / sqrt(
                                            1 - steering_track_ * rear_tmp * cos(rear_steering) +
                                            pow(steering_track_ * rear_tmp / 2, 2));
  const double rear_right_tmp = rear_tmp / sqrt(
                                             1 + steering_track_ * rear_tmp * cos(rear_steering) +
                                             pow(steering_track_ * rear_tmp / 2, 2));

  const double rl_speed_tmp = rl_speed * (1 / (1 - wheel_steering_y_offset_ * rear_left_tmp));
  const double rr_speed_tmp = rr_speed * (1 / (1 - wheel_steering_y_offset_ * rear_right_tmp));

  const double rear_linear_speed = wheel_radius_ * copysign(1.0, rl_speed_tmp + rr_speed_tmp) *
                                   sqrt(
                                     (pow(rl_speed_tmp, 2) + pow(rr_speed_tmp, 2)) /
                                     (2 + pow(steering_track_ * rear_tmp, 2) / 2.0));

  angular_ = (front_linear_speed * front_tmp + rear_linear_speed * rear_tmp) / 2.0;

  const double linear_x_ =
    (front_linear_speed * cos(front_steering) + rear_linear_speed * cos(rear_steering)) / 2.0;
  const double linear_y_ =
    (front_linear_speed * sin(front_steering) - wheel_base_ * angular_ / 2.0 +
     rear_linear_speed * sin(rear_steering) + wheel_base_ * angular_ / 2.0) /
    2.0;

  const double linear_velocity =
    copysign(1.0, rear_linear_speed) * sqrt(pow(linear_x_, 2) + pow(linear_y_, 2));

  /// Integrate odometry:
  // integrateXY(linear_x_*dt, linear_y_*dt, angular_*dt);

  // linear_accel_acc_((linear_vel_prev_ - linear_velocity)/dt);
  // linear_vel_prev_ = linear_velocity;
  // linear_jerk_acc_((linear_accel_prev_ - bacc::rolling_mean(linear_accel_acc_))/dt);
  // linear_accel_prev_ = bacc::rolling_mean(linear_accel_acc_);
  // front_steer_vel_acc_((front_steer_vel_prev_ - front_steering)/dt);
  // front_steer_vel_prev_ = front_steering;
  // rear_steer_vel_acc_((rear_steer_vel_prev_ - rear_steering)/dt);
  // rear_steer_vel_prev_ = rear_steering;
  return update_odometry(linear_velocity, angular_, dt);
}

bool Odometry::updateFromVelocity(double left_vel, double right_vel, const rclcpp::Time& time)
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

void Odometry::updateOpenLoop(double linear, double angular, const rclcpp::Time& time)
{
  /// Save last linear and angular velocity:
  linear_ = linear;
  angular_ = angular;

  /// Integrate odometry:
  const double dt = time.seconds() - timestamp_.seconds();

  timestamp_ = time;
  integrateExact(linear * dt, angular * dt);
}

void Odometry::resetOdometry()
{
  x_ = 0.0;
  y_ = 0.0;
  heading_ = 0.0;
}

void Odometry::setWheelParams(
  double steering_track, double wheel_radius, double wheel_base, double wheel_steering_y_offset)
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

}  // namespace strafe_controller
