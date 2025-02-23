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
//#include "realtime_tools/realtime_buffer.h"
//#include "realtime_tools/realtime_publisher.h"
#include "tf2_msgs/msg/tf_message.hpp"
#include "tf2/LinearMath/Scalar.h"
#include "nova_ik_controller/speed_limiter.hpp"
#include <Eigen/Dense>
#include <tf2/LinearMath/Transform.h>

// To test in development, run from the root nova_ik_controller dir:
// generate_parameter_library_cpp include/nova_ik_controller/nova_ik_controller_parameters.hpp src/nova_ik_controller_parameter.yaml
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

  // TODO: Reset twistmapper output pose to state interface reported positions
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

  /// @brief Calculates the IK given a frame, pose and set of lengths.
  /// @brief See Keenan's IK notes.
  /// @param frame is ???
  /// @param pose is a struct combining a position (x,y,z) and a quaternion (x,y,z,w).
  /// @param Lengths are [fill this].
  /// @returns an array of joint angles in joint space for each of J1 through J6, through joints.
  std::array<double, 6> calculate_ik(tf2::Transform pose, std::array<double, 3> lengths);

  // Helper functions to cut down on code reuse. Gets sin/cos/tan from degrees instead of radians.
  tf2Scalar sind(tf2Scalar angle) const { return tf2Sin(tf2Radians(angle)); }
  tf2Scalar cosd(tf2Scalar angle) const { return tf2Cos(tf2Radians(angle)); }
  tf2Scalar tand(tf2Scalar angle) const { return tf2Tan(tf2Radians(angle)); }

  // substitutes values into our DH table.
  // see: https://www.notion.so/Inverse-Kinematics-ddfe35179c1f4959850bd28b2195be8a
  // equivalent line: DHs = [cos(the) -sin(the) 0 a; sin(the)*cos(alp) cos(the)*cos(alp) -sin(alp) -sin(alp)*d; sin(the)*sin(alp) cos(the)*sin(alp) cos(alp) cos(alp)*d; 0 0 0 1];
  Eigen::Matrix4d sub_dh(double alp, double a, double d, double the) const
  {
    return Eigen::Matrix4d {
      { cos(the), 			-sin(the), 			0, 			a },
      { sin(the)*cos(alp),	cos(the)*cos(alp), 	-sin(alp), 	-sin(alp)*d },
      { sin(the)*sin(alp),	cos(the)*sin(alp),	cos(alp),	cos(alp)*d },
      { 0,					0,					0,			1 }
    };
  }

  void teleop_callback(std::shared_ptr<geometry_msgs::msg::TwistStamped> msg);

protected:
  struct JointHandle
  {
    std::string name;
    std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
    // TODO: Add state interfaces to be used by the twistmapper for resetting on activation (or after activation upon receiving the first bit of meaningful data)
    // SpeedLimiter speed_limiter;
    // float target_direction = 0.0;
    // float best_effort_rotational_velocity = 0.0;
    // store per joint odometry here maybe?
  };

  controller_interface::CallbackReturn configure_joints(
    const std::vector<std::string> &joint_names,
    std::vector<JointHandle> &registered_handles);

  // Helpers
  std::string joint_to_command_interface_name(const std::string& joint_name) const;

  rclcpp::Node node;

  // TODO: change this message when we get one

  std::vector<JointHandle> registered_joint_handles_;

  // Parameters from ROS for nova_diff_drive_controller
  std::shared_ptr<ParamListener> param_listener_;
  Params params_;

  // Twistmapper

  // TODO: Initialize twist stamped topic and write callback
  rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr twist_stamped_sub = nullptr;
  realtime_tools::RealtimeBox<std::shared_ptr<geometry_msgs::msg::TwistStamped>> received_twist_stamped_ptr{nullptr};
  /// Result of the twistmapper, and input to IK. Desired position and orientation of the end effector relative to the base.
  tf2::Transform _twistmapper_pose = tf2::Transform();
  /// True when initial state has been received to confirm that the value in _twistmapper_pose is safe and valid
  // TODO: Make invalid when deactivated
  bool _twistmapper_pose_validated = false;

  // TODO: Detection of initial state, setting initial twistmapper pose

  // TODO: Timeout and halt for safety!

  // TODO: Implement
  void update_twistmapper(const rclcpp::Time &time, const rclcpp::Duration &period);

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
