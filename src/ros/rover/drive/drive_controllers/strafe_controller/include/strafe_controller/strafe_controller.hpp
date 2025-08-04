#ifndef STRAFE_CONTROLLER__STRAFE_CONTROLLER_HPP_
#define STRAFE_CONTROLLER__STRAFE_CONTROLLER_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <vector>
#include <tuple>

#include "hardware_interface/handle.hpp"
#include "controller_interface/controller_interface.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "tf2_msgs/msg/tf_message.hpp"
#include "std_srvs/srv/empty.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_box.h"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"

#include "nova_controller_common/speed_limiter.hpp"
#include "nova_controller_common/hardware_interface_wrapper.hpp"
#include "strafe_controller/odometry.hpp"
#include "strafe_controller_parameters.hpp"

namespace strafe_controller
{

class StrafeController : public controller_interface::ControllerInterface
{
public:
  StrafeController();

  controller_interface::InterfaceConfiguration command_interface_configuration() const override;
  controller_interface::InterfaceConfiguration state_interface_configuration() const override;

  controller_interface::return_type update(
    const rclcpp::Time& time, const rclcpp::Duration& period) override;

  controller_interface::CallbackReturn on_init() override;

  controller_interface::CallbackReturn on_configure(
    const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_activate(
    const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_deactivate(
    const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_cleanup(
    const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_error(
    const rclcpp_lifecycle::State& previous_state) override;

  controller_interface::CallbackReturn on_shutdown(
    const rclcpp_lifecycle::State& previous_state) override;

protected:
  const char* drive_feedback_type() const;
  const char* pivot_feedback_type() const;
  const char* DRIVE_COMMAND_TYPE_;
  const char* PIVOT_COMMAND_TYPE_;

  bool reset();
  void reset_buffers();
  void halt();

  bool is_active_ = false;
  bool is_halted_ = false;

  // Parameters from ROS for strafe_controller
  std::shared_ptr<ParamListener> param_listener_;
  std::shared_ptr<Params> params_;

  size_t wheels_per_side_;
  const size_t PIVOTS_PER_SIDE_;

  std::unique_ptr<nova_controller_common::HardwareInterfaceWrapper> hwif_wrapper_;
  std::unique_ptr<Odometry> odometry_;

  std::deque<double> previous_linear_velocities_;   // last two linear velocity commands

  // Limiters
  nova_controller_common::SpeedLimiter limiter_linear_;

  // Timeout to consider cmd_vel commands old
  rclcpp::Duration cmd_vel_timeout_ = rclcpp::Duration::from_seconds(0.5);

  // Subscriber and realtime buffer for received TwistStamped messages
  rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr twist_subscriber_;
  realtime_tools::RealtimeBuffer<std::shared_ptr<geometry_msgs::msg::TwistStamped>>
    received_twist_msg_ptr_;

  // Publisher and realtime buffer for commanded TwistStamped messages
  std::shared_ptr<rclcpp::Publisher<geometry_msgs::msg::TwistStamped>> commanded_twist_publisher_;
  std::shared_ptr<realtime_tools::RealtimePublisher<geometry_msgs::msg::TwistStamped>>
    realtime_commanded_twist_publisher_;
};

}  // namespace strafe_controller

#endif  // STRAFE_CONTROLLER__STRAFE_CONTROLLER_HPP_
