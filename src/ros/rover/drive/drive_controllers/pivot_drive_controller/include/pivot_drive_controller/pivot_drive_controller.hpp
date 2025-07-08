#ifndef PIVOT_DRIVE_CONTROLLER__PIVOT_DRIVE_CONTROLLER_HPP_
#define PIVOT_DRIVE_CONTROLLER__PIVOT_DRIVE_CONTROLLER_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include <utility>

#include "hardware_interface/handle.hpp"
#include "controller_interface/controller_interface.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "std_srvs/srv/empty.hpp"
#include "std_srvs/srv/trigger.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"

#include "nova_controller_common/speed_limiter.hpp"
#include "nova_controller_common/position_limiter.hpp"
#include "nova_controller_common/blcmd_wrapper.hpp"
#include "pivot_drive_controller/odometry.hpp"
#include "pivot_drive_controller_parameters.hpp"

namespace pivot_drive_controller
{
class PivotDriveController : public controller_interface::ControllerInterface
{
public:
  PivotDriveController();

  controller_interface::InterfaceConfiguration command_interface_configuration() const override;
  controller_interface::InterfaceConfiguration state_interface_configuration() const override;

  controller_interface::return_type update(const rclcpp::Time& time, const rclcpp::Duration& period) override;

  controller_interface::CallbackReturn on_init() override;

  controller_interface::CallbackReturn on_configure(const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_activate(const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_deactivate(const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_cleanup(const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_error(const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_shutdown(const rclcpp_lifecycle::State& previous_state) override;

private:
  const char* drive_feedback_type() const;
  const char* pivot_feedback_type() const;

  std::pair<double, double> get_pivot_angles_from_radius(float radius, int dir);

  bool reset();
  void reset_buffers();
  void halt();

  bool is_halted_ = false;

  // Parameters from ROS for pivot_drive_controller
  std::shared_ptr<ParamListener> param_listener_;
  Params params_;

  double zero_radius_;   // radius of the circle the rover makes with its wheels when turning on the spot
  double offset_angle_;  // angle at which the wheels are initially offset

  std::unique_ptr<nova_controller_common::BLCMDWrapper> blcmd_wrapper_;
  
  std::queue<double> previous_linear_velocities;   // last two linear velocity commands
  std::queue<double> previous_angular_positions_;  // last three angular position commands
  
  // Limiters
  nova_controller_common::SpeedLimiter limiter_linear_;
  nova_controller_common::PositionLimiter limiter_angular_;

  // Timeout to consider cmd_vel commands old
  std::chrono::milliseconds cmd_vel_timeout_;

  Odometry odometry_;
  std::shared_ptr<rclcpp::Publisher<nav_msgs::msg::Odometry>> odometry_publisher_;
  std::shared_ptr<realtime_tools::RealtimePublisher<nav_msgs::msg::Odometry>> realtime_odometry_publisher_;

  std::shared_ptr<rclcpp::Publisher<tf2_msgs::msg::TFMessage>> odometry_transform_publisher_;
  std::shared_ptr<realtime_tools::RealtimePublisher<tf2_msgs::msg::TFMessage>> realtime_odometry_transform_publisher_;

  bool is_active_ = false;
  rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr twist_subscriber_;
  realtime_tools::RealtimeBuffer<std::shared_ptr<geometry_msgs::msg::TwistStamped>> received_twist_msg_ptr_;

  // Publish rate limiter
  rclcpp::Duration publish_period_ = rclcpp::Duration::from_nanoseconds(0);
  rclcpp::Time previous_publish_timestamp_{0, 0, RCL_CLOCK_UNINITIALIZED};

  static constexpr const char* DRIVE_COMMAND_TYPE;
  static constexpr const char* PIVOT_COMMAND_TYPE;
};

}  // namespace pivot_drive_controller

#endif  // PIVOT_DRIVE_CONTROLLER__PIVOT_DRIVE_CONTROLLER_HPP_
