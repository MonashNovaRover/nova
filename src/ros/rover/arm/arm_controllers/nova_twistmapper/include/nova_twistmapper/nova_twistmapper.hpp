/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

A ros2_control controller for Banksia's robotic
  arm payload, which takes TwistStamped messages,
  to translate a virtual target Transform reached
  by inverse kinematics.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CONTROLLER: nova_twistmapper/NovaTwistmapper
SUBSCRIPTIONS:
  - /arm_ik_twist_stamped [geometry_msgs/TwistStamped]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:  nova_twistmapper
AUTHOR:   Bailey Chessum
CREATION: 13/04/2025
EDITED:   21/04/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#ifndef NOVA_TWISTMAPPER__NOVA_TWISTMAPPER_HPP_
#define NOVA_TWISTMAPPER__NOVA_TWISTMAPPER_HPP_

#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

#include <Eigen/Geometry>

#include "controller_interface/controller_interface.hpp"
#include "geometry_msgs/msg/twist_stamped.hpp"
#include "hardware_interface/handle.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_box.hpp"
#include "tf2_ros/transform_broadcaster.h"

#include "arm_kinematics/collision/collision_manager.hpp"
#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"
#include "arm_kinematics/inverse/inverse_kinematics_plugin.hpp"
#include "arm_kinematics/plugin_loader.hpp"
#include "arm_kinematics/utilities/aliases.hpp"
#include "arm_kinematics/utilities/expected.hpp"
#include "visibility_control.h"

// To test in development, run from the root nova_twistmapper dir:
// generate_parameter_library_cpp include/nova_twistmapper/nova_twistmapper_parameters.hpp src/nova_twistmapper_parameter.yaml
#include "nova_twistmapper_parameters.hpp"

namespace nova_twistmapper
{
class NovaTwistmapper : public controller_interface::ControllerInterface
{
public:
  NovaTwistmapper();

  controller_interface::InterfaceConfiguration command_interface_configuration() const override;

  controller_interface::InterfaceConfiguration state_interface_configuration() const override;

  controller_interface::return_type update(
    const rclcpp::Time & time,
    const rclcpp::Duration & period) override;

  controller_interface::CallbackReturn on_init() override;

  controller_interface::CallbackReturn on_configure(
    const rclcpp_lifecycle::State & previous_state) override;

  controller_interface::CallbackReturn on_activate(
    const rclcpp_lifecycle::State & previous_state) override;

  controller_interface::CallbackReturn on_deactivate(
    const rclcpp_lifecycle::State & previous_state) override;

  controller_interface::CallbackReturn on_cleanup(
    const rclcpp_lifecycle::State & previous_state) override;

  controller_interface::CallbackReturn on_error(
    const rclcpp_lifecycle::State & previous_state) override;

  controller_interface::CallbackReturn on_shutdown(
    const rclcpp_lifecycle::State & previous_state) override;

protected:
  enum class TwistmapperMode
  {
    Position,
    Velocity,
  };

  enum class TwistResolutionStatus
  {
    Valid,
    NoMessage,
    StaleMessage,
    InvalidFrame,
  };

  struct ResolvedTwist
  {
    TwistResolutionStatus status = TwistResolutionStatus::NoMessage;
    arm_kinematics::Twistd base_twist = arm_kinematics::Twistd::Zero();
  };

  struct JointHandle
  {
    std::string name;
    std::reference_wrapper<const hardware_interface::LoanedStateInterface> state_pos;
    std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
  };

  struct Kinematics
  {
    arm_kinematics::PluginLoader loader;
    arm_kinematics::ForwardKinematicsPlugin::SharedPtr fk;
    arm_kinematics::InverseKinematicsPlugin::SharedPtr ik;
    arm_kinematics::CollisionManager collision_manager;
    arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr ee_tree;
  };

  struct PositionRuntime
  {
    Eigen::Isometry3d target_pose = Eigen::Isometry3d::Identity();
    std::vector<double> solution_positions{};
  };

  struct VelocityRuntime
  {
    Eigen::Isometry3d current_ee_pose = Eigen::Isometry3d::Identity();
    std::vector<double> solution_velocities{};
  };

  using ModeRuntime = std::variant<PositionRuntime, VelocityRuntime>;

  controller_interface::CallbackReturn configure_joints();

  [[nodiscard]] TwistmapperMode twistmapper_mode() const noexcept;

  [[nodiscard]] const char * joint_command_type() const noexcept;

  void read_state_pos_values(std::vector<double> & joint_values) const;

  [[nodiscard]] ResolvedTwist resolve_base_twist(
    const std::vector<double> & seed_state,
    const rclcpp::Time & time);

  [[nodiscard]] Eigen::Isometry3d integrate_target_pose(
    const arm_kinematics::Twistd & base_twist,
    const rclcpp::Duration & period,
    const Eigen::Isometry3d & current_target_pose) const;

  controller_interface::return_type update_position_mode(
    PositionRuntime & runtime,
    const rclcpp::Time & time,
    const rclcpp::Duration & period);

  controller_interface::return_type update_velocity_mode(
    VelocityRuntime & runtime,
    const rclcpp::Time & time,
    const rclcpp::Duration & period);

  bool write_commands(const std::vector<double> & commands);

  bool write_zero_velocity_commands();

  void log_self_intersection_pairs(
    const char * message_prefix,
    const std::vector<std::pair<size_t, size_t>> & colliding_pairs);

  tl::expected<arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr, std::string>
  make_single_frame_tree(const std::string & frame_name) const;

  bool reset();

  void halt();

  void publish_to_tf2(
    const rclcpp::Time & time,
    const Eigen::Isometry3d & pose,
    const std::string & name = "");

  [[nodiscard]] bool vector_is_finite(const std::vector<double> & values) const noexcept;

  std::vector<JointHandle> registered_joint_handles_{};

  std::shared_ptr<ParamListener> param_listener_{};
  Params params_;

  rclcpp::Subscription<geometry_msgs::msg::TwistStamped>::SharedPtr twist_stamped_sub_{nullptr};
  realtime_tools::RealtimeBox<std::shared_ptr<geometry_msgs::msg::TwistStamped>> received_twist_stamped_ptr_{nullptr};
  realtime_tools::RealtimeBox<arm_kinematics::ForwardKinematicsPlugin::Tree::SharedPtr> active_twist_frame_tree_{nullptr};
  std::string last_frame_id_{};
  std::string debug_twist_frame_child_frame_id_{};

  std::shared_ptr<tf2_ros::TransformBroadcaster> twistmapper_pose_tf_broadcaster_{};

  std::optional<Kinematics> kinematics_{};
  std::optional<ModeRuntime> mode_runtime_{};
  arm_kinematics::Isometry3dVector fk_pose_buffer_{
    1,
    Eigen::Isometry3d::Identity()};
  std::vector<double> current_joint_state_values_{};
  std::vector<double> joint_values_scratch_{};
  std::vector<double> predicted_joint_positions_{};
  std::vector<std::pair<size_t, size_t>> colliding_pairs_scratch_{};

  bool subscriber_is_active_ = false;
  rclcpp::Time previous_update_timestamp_{0};
  bool is_halted = false;
};
}  // namespace nova_twistmapper

#endif  // NOVA_TWISTMAPPER__NOVA_TWISTMAPPER_HPP_
