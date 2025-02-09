#ifndef NOVA_IK_CONTROLLER__NOVA_IK_CONTROLLER_HPP_
#define NOVA_IK_CONTROLLER__NOVA_IK_CONTROLLER_HPP_

#include <chrono>
#include <cmath>
#include <memory>
#include <queue>
#include <string>
#include <vector>

#include "visibility_control.h"
#include "controller_interface/controller_interface.hpp"
#include "geometry_msgs/msg/twist.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "hardware_interface/handle.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "rclcpp/node.hpp"
#include "realtime_tools/realtime_box.h"
#include "realtime_tools/realtime_buffer.h"
#include "realtime_tools/realtime_publisher.h"
#include "tf2_msgs/msg/tf_message.hpp"
#include "nova_ik_controller/speed_limiter.hpp"

#include "nova_ik_controller_parameters.hpp"

namespace nova_ik_controller
{
  class NovaIKController : public controller_interface::ControllerInterface
  {
  public:
    NovaIKController();

    controller_interface::InterfaceConfiguration command_interface_configuration() const override;

    controller_interface::InterfaceConfiguration state_interface_configuration() const override;

    controller_interface::return_type update(
        const rclcpp::Time &time, const rclcpp::Duration &period) override;

    controller_interface::CallbackReturn on_init() override;

    controller_interface::CallbackReturn on_configure(
        const rclcpp_lifecycle::State &previous_state) override;

    controller_interface::CallbackReturn on_activate(
        const rclcpp_lifecycle::State &previous_state) override;

    controller_interface::CallbackReturn on_deactivate(
        const rclcpp_lifecycle::State &previous_state) override;

    controller_interface::CallbackReturn on_cleanup(
        const rclcpp_lifecycle::State &previous_state) override;

    controller_interface::CallbackReturn on_error(
        const rclcpp_lifecycle::State &previous_state) override;

    controller_interface::CallbackReturn on_shutdown(
        const rclcpp_lifecycle::State &previous_state) override;
	
	void calculate_ik(tf2_msgs::msg::TFMessage frame, geometry_msgs::msg::Pose wristPose, geometry_msgs::msg::Pose effPose);
	
	void teleop_callback(tf2_msgs::msg::TFMessage msg);

  protected:
    struct JointHandle
    {
      std::string name;
      std::reference_wrapper<const hardware_interface::LoanedStateInterface> state;
      std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
      SpeedLimiter speed_limiter;
      float target_direction = 0.0;
      float best_effort_rotational_velocity = 0.0;
      // store per joint odometry here maybe?
    };
	
	rclcpp::Node node;

	// TODO: change this message when we get one
	rclcpp::Subscription<tf2_msgs::msg::TFMessage>::SharedPtr teleop_sub;

    controller_interface::CallbackReturn configure_joints(
        const std::vector<std::string> &joint_names,
        std::vector<JointHandle> &registered_handles, const char *feedback_type);

    const char *joint_feedback_type() const;

    std::vector<JointHandle> registered_joint_handles_;

    // Parameters from ROS for nova_diff_drive_controller
    std::shared_ptr<ParamListener> param_listener_;
    Params params_;

    // Timeout to consider cmd_vel commands old
    std::chrono::milliseconds cmd_vel_timeout_{500};
    bool subscriber_is_active_ = false; // not sure what this is for yet
    rclcpp::Time previous_update_timestamp_{0};

    // publish rate limiter
    double publish_rate_ = 50.0;
    rclcpp::Duration publish_period_ = rclcpp::Duration::from_nanoseconds(0);
    rclcpp::Time previous_publish_timestamp_{0, 0, RCL_CLOCK_UNINITIALIZED};
    bool is_halted = false;
    bool reset();
    void halt();

  };
} // namespace nova_ik_controller
#endif // NOVA_IK_CONTROLLER__NOVA_IK_CONTROLLER_HPP_
