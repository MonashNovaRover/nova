#ifndef NOVA_ARM_CONTROLLER__NOVA_ARM_CONTROLLER_HPP_
#define NOVA_ARM_CONTROLLER__NOVA_ARM_CONTROLLER_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "controller_interface/controller_interface.hpp"
#include "controller_interface/chainable_controller_interface.hpp"
#include "controller_interface/controller_interface.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "hardware_interface/handle.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_box.h"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"
#include "tf2_msgs/msg/tf_message.hpp"
#include <nova_interfaces/msg/arm_fk_velocity_targets.hpp>
#include "joint_limits/joint_saturation_limiter.hpp"
#include "trajectory_msgs/msg/joint_trajectory_point.hpp"

#include "nova_arm_controller_parameters.hpp"

namespace nova_arm_controller
{
class NovaArmController : public controller_interface::ChainableControllerInterface
{
public:
  //NovaArmController();

  controller_interface::InterfaceConfiguration command_interface_configuration() const override;

  controller_interface::InterfaceConfiguration state_interface_configuration() const override;

  std::vector<hardware_interface::CommandInterface> on_export_reference_interfaces() override;

  controller_interface::return_type update_and_write_commands(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

  controller_interface::CallbackReturn on_init() override;

  controller_interface::CallbackReturn on_configure(
      const rclcpp_lifecycle::State &previous_state) override;

  controller_interface::CallbackReturn on_activate(
      const rclcpp_lifecycle::State &previous_state) override;

  controller_interface::CallbackReturn on_deactivate(
      const rclcpp_lifecycle::State &previous_state) override;

  bool on_set_chained_mode(bool chained_mode) override;

  controller_interface::CallbackReturn on_cleanup(
      const rclcpp_lifecycle::State &previous_state) override;

  controller_interface::CallbackReturn on_error(
      const rclcpp_lifecycle::State &previous_state) override;

  controller_interface::CallbackReturn on_shutdown(
      const rclcpp_lifecycle::State &previous_state) override;

protected:
  struct JointHandle
  {
    std::string name;
    std::reference_wrapper<const hardware_interface::LoanedStateInterface> state_pos;
    std::reference_wrapper<const hardware_interface::LoanedStateInterface> state_vel;
    std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
    // SpeedLimiter speed_limiter;
    // float target_direction = 0.0;
    // float best_effort_rotational_velocity = 0.0;
    // store per joint odometry here maybe?
  };

  // if you update ros, the template changes to joint_limits::JointControlInterfacesData btw
  joint_limits::JointSaturationLimiter<joint_limits::JointLimits> joint_limiter;

  controller_interface::CallbackReturn configure_joints(
      const std::vector<std::string> &joint_names,
      std::vector<JointHandle> &registered_handles);

  controller_interface::return_type update_reference_from_subscribers(
      const rclcpp::Time & time, const rclcpp::Duration & period) override;
  controller_interface::return_type update_velocity_reference_from_subscribers();

  const char *joint_feedback_type() const;
  const char *joint_command_type() const;

  /// Joints being used by the controller. Order should match that of the joint name definitions in parameters
  std::vector<JointHandle> registered_joint_handles_;

  // Parameters from ROS for nova_arm_controller
  std::shared_ptr<ParamListener> param_listener_{};
  Params params_{};

  // Timeout to consider cmd_vel commands old
  std::chrono::milliseconds cmd_vel_timeout_{500};
  bool subscriber_is_active_ = false; // not sure what this is for yet

  // Subscription
  rclcpp::Subscription<nova_interfaces::msg::ArmFkVelocityTargets>::SharedPtr input_subscriber_ = nullptr;
  realtime_tools::RealtimeBox<std::shared_ptr<nova_interfaces::msg::ArmFkVelocityTargets>> received_msg_ptr_{nullptr};

  rclcpp::Time previous_update_timestamp_{0};

  // publish rate limiter
  double publish_rate_ = 50.0;
  rclcpp::Duration publish_period_ = rclcpp::Duration::from_nanoseconds(0);
  rclcpp::Time previous_publish_timestamp_{0, 0, RCL_CLOCK_UNINITIALIZED};
  bool is_halted = false;
  bool reset();
  void halt();

  void get_joint_states(joint_limits::JointLimitsStateDataType &);

};
} // namespace nova_arm_controller
#endif // NOVA_ARM_CONTROLLER__NOVA_ARM_CONTROLLER_HPP_
