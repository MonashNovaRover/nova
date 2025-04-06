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
#include "controller_interface/chainable_controller_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "nav_msgs/msg/odometry.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "rclcpp/node.hpp"
#include "realtime_tools/realtime_box.h"
#include "tf2_msgs/msg/tf_message.hpp"
#include "tf2/LinearMath/Scalar.h"
#include "nova_ik_controller/speed_limiter.hpp"
#include <Eigen/Dense>
#include <tf2/LinearMath/Transform.h>
#include <tf2_ros/transform_broadcaster.h>
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"

// To test in development, run from the root nova_ik_controller dir:
// generate_parameter_library_cpp include/nova_ik_controller/nova_ik_controller_parameters.hpp src/nova_ik_controller_parameter.yaml
#include "nova_ik_controller_parameters.hpp"

namespace nova_ik_controller
{
class NovaIKController : public controller_interface::ChainableControllerInterface
{
public:
  NovaIKController();

  controller_interface::InterfaceConfiguration command_interface_configuration() const override;

  controller_interface::InterfaceConfiguration state_interface_configuration() const override;

  std::vector<hardware_interface::CommandInterface> on_export_reference_interfaces() override;

  controller_interface::return_type update_and_write_commands(
    const rclcpp::Time& time, const rclcpp::Duration& period) override;

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

  controller_interface::return_type update_reference_from_subscribers(
    const rclcpp::Time & time, const rclcpp::Duration & period) override;

  /// @brief Calculates the IK given a frame, pose and set of lengths.
  /// @brief See Keenan's IK notes.
  /// @param frame is ???
  /// @param pose is a struct combining a position (x,y,z) and a quaternion (x,y,z,w).
  /// @param Lengths are [fill this].
  /// @returns an array of joint angles in joint space for each of J1 through J6, through joints.
  std::array<double, 6> calculate_ik(tf2::Transform pose, std::array<double, 3> lengths);

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

protected:
  struct JointHandle
  {
    std::string name;
    std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
    // SpeedLimiter speed_limiter;
  };

  controller_interface::CallbackReturn configure_joints(
    const std::vector<std::string> &joint_names,
    std::vector<JointHandle> &registered_handles);

  // Helpers
  std::string joint_to_command_interface_name(const std::string& joint_name) const;

  std::vector<JointHandle> registered_joint_handles_;

  // Parameters from ROS for nova_diff_drive_controller
  std::shared_ptr<ParamListener> param_listener_;
  Params params_;

  bool reset();
};
} // namespace nova_ik_controller
#endif // NOVA_IK_CONTROLLER__NOVA_IK_CONTROLLER_HPP_
