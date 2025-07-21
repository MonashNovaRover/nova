#ifndef FOUR_STEERING_CONTROLLER__ODOMETRY_HPP_
#define FOUR_STEERING_CONTROLLER__ODOMETRY_HPP_

#include <cmath>

#include "rclcpp/time.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/macros.hpp"
#include "rcpputils/rolling_mean_accumulator.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/logging.hpp"

namespace pivot_drive_controller 
{
class Odometry
{
public:
  explicit Odometry(size_t velocity_rolling_window_size = 10);

  void init(const rclcpp::Time & time);
  bool update(const double &fl_speed, const double &fr_speed,
              const double &rl_speed, const double &rr_speed,
              double front_steering, double rear_steering, const double &time);
  bool updateFromVelocity(double left_vel, double right_vel, const rclcpp::Time & time);
  void updateOpenLoop(double linear_vel, double angular_vel, const rclcpp::Time & time);
  void resetOdometry();
  bool update_odometry(const double linear_velocity, const double angular, const double dt);

  double getX() const { return x_; }
  double getY() const { return y_; }
  double getHeading() const { return heading_; }
  double getLinear() const { return linear_; }
  double getLinearX() const { return linear_x_; }
  double getLinearY() const { return linear_y_; }

  double getAngular() const { return angular_; }

  void setWheelParams(double steering_track, double wheel_radius,
                      double wheel_base, double wheel_steering_y_offset);
  void setVelocityRollingWindowSize(size_t velocity_rolling_window_size);

private:
  using RollingMeanAccumulator = rcpputils::RollingMeanAccumulator<double>;
  void integrateXY(double linear_x, double linear_y, double angular);
  void integrateRungeKutta2(double linear, double angular);
  void integrateExact(double linear, double angular);
  void resetAccumulators();

  // Current timestamp:
  rclcpp::Time timestamp_;

  // Current pose:
  double x_;        //   [m]
  double y_;        //   [m]
  double heading_;  // [rad]

  // Current velocity:
  double linear_;   //   [m/s]
  double angular_;  // [rad/s]
  double linear_x_;
  double linear_y_;
  // Wheel kinematic parameters [m]:
  double wheel_base_;
  double wheel_radius_;
  double wheel_steering_y_offset_;
  double steering_track_;
  // Previous wheel and pivot positions [rad]:
  double left_wheel_old_pos_;
  double right_wheel_old_pos_;
  double left_pivot_old_pos_;
  double right_pivot_old_pos_;

  // Rolling mean accumulators for the linear and angular velocities:
  size_t velocity_rolling_window_size_;
  RollingMeanAccumulator linear_accumulator_;
  RollingMeanAccumulator angular_accumulator_;
};

}  // namespace four_steering_controller

#endif  // FOUR_STEERING_CONTROLLER__ODOMETRY_HPP_
