/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

A ros2_control controller for Banksia's robotic
  arm payload, which generates joint-space paths
  between two virtual target end effector poses.
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CONTROLLER: nova_path_planner/NovaPathPlanner
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:  nova_path_planner
AUTHOR:   Bailey Chessum
EDITED:   21/04/2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#ifndef NOVA_PATH_PLANNER__NOVA_PATH_PLANNER_HPP_
#define NOVA_PATH_PLANNER__NOVA_PATH_PLANNER_HPP_

#include <atomic>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <vector>

#include <Eigen/Geometry>

#include "controller_interface/controller_interface.hpp"
#include "hardware_interface/handle.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_action/rclcpp_action.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "realtime_tools/realtime_box.hpp"
#include "tf2_ros/transform_broadcaster.h"

#include "arm_kinematics/collision/collision_manager.hpp"
#include "arm_kinematics/forward/forward_kinematics_plugin.hpp"
#include "arm_kinematics/inverse/inverse_kinematics_plugin.hpp"
#include "arm_kinematics/plugin_loader.hpp"
#include "arm_kinematics/utilities/aliases.hpp"
#include "nova_interfaces/action/arm_plan_path.hpp"
#include "visibility_control.h"

// To test in development, run from the root nova_path_planner dir:
// generate_parameter_library_cpp include/nova_path_planner/nova_path_planner_parameters.hpp src/nova_path_planner_parameter.yaml
#include "nova_path_planner_parameters.hpp"

namespace nova_path_planner
{
class NovaPathPlanner : public controller_interface::ControllerInterface
{
public:
  using ArmPlanPath = nova_interfaces::action::ArmPlanPath;
  using GoalHandleArmPlanPath = rclcpp_action::ServerGoalHandle<ArmPlanPath>;
  using Vector3d = Eigen::Vector3d;

  NovaPathPlanner();

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

  struct PlannedPath
  {
    std::vector<std::vector<double>> points{};
  };

  controller_interface::CallbackReturn configure_joints();

  void read_state_pos_values(std::vector<double> & joint_values) const;

  void get_state_pos_values_non_rt(std::vector<double> & joint_values) const;

  void publish_target_pose_to_tf2(
    const rclcpp::Time & time,
    const Eigen::Isometry3d & pose);

  bool reset();

  void halt();

  rclcpp_action::GoalResponse handle_action_goal(
    const rclcpp_action::GoalUUID & uuid,
    std::shared_ptr<const ArmPlanPath::Goal> goal);

  void handle_action_accepted(const std::shared_ptr<GoalHandleArmPlanPath> & goal_handle);

  rclcpp_action::CancelResponse handle_action_cancelled(
    const std::shared_ptr<GoalHandleArmPlanPath> & goal_handle);

  void abort_action(
    const std::shared_ptr<GoalHandleArmPlanPath> & goal_handle,
    const std::shared_ptr<ArmPlanPath::Result> & result);

  void execute_action(std::shared_ptr<GoalHandleArmPlanPath> goal_handle);

  bool try_get_pose_from_forward_kinematics(
    const std::vector<double> & joint_positions,
    Eigen::Isometry3d & result);

  bool generate_path(
    const Eigen::Isometry3d & start,
    const Eigen::Isometry3d & end,
    std::vector<double> & last_pushed_pose,
    double speed);

  void clear_path_execution();

  [[nodiscard]] bool vector_is_finite(const std::vector<double> & values) const noexcept;

  static Vector3d lerp(const Vector3d & a, const Vector3d & b, const double & t);
  static Vector3d lerp2(const Vector3d & a, const Vector3d & b, const Vector3d & c, const double & t);
  static Vector3d lerp3(
    const Vector3d & a,
    const Vector3d & b,
    const Vector3d & c,
    const Vector3d & d,
    const double & t);

  static Eigen::Quaterniond slerp(
    const Eigen::Quaterniond & a,
    const Eigen::Quaterniond & b,
    const double & t);
  static Eigen::Quaterniond slerp2(
    const Eigen::Quaterniond & a,
    const Eigen::Quaterniond & b,
    const Eigen::Quaterniond & c,
    const double & t);
  static Eigen::Quaterniond slerp3(
    const Eigen::Quaterniond & a,
    const Eigen::Quaterniond & b,
    const Eigen::Quaterniond & c,
    const Eigen::Quaterniond & d,
    const double & t);

  static Eigen::Isometry3d lerp3(
    Eigen::Isometry3d a,
    Eigen::Isometry3d b,
    Eigen::Isometry3d c,
    Eigen::Isometry3d d,
    double t);

  std::vector<JointHandle> registered_joint_handles_{};

  std::shared_ptr<ParamListener> param_listener_{};
  Params params_;

  rclcpp_action::Server<ArmPlanPath>::SharedPtr action_server_{};
  realtime_tools::RealtimeBox<std::shared_ptr<const PlannedPath>> pending_path_ptr_{nullptr};
  std::shared_ptr<const PlannedPath> active_path_{};
  std::size_t active_path_index_{0};
  std::atomic<bool> clear_path_requested_{false};
  std::atomic<std::size_t> remaining_path_points_{0};
  std::atomic<bool> is_path_being_executed_{false};
  std::optional<std::thread> action_thread_{};
  std::shared_ptr<tf2_ros::TransformBroadcaster> target_pose_tf_broadcaster_{};
  Eigen::Isometry3d target_pose_ = Eigen::Isometry3d::Identity();
  bool has_target_pose_ = false;

  std::optional<Kinematics> kinematics_{};
  arm_kinematics::Isometry3dVector fk_pose_buffer_{
    1,
    Eigen::Isometry3d::Identity()};
  std::vector<double> current_joint_state_values_{};
  std::vector<double> action_joint_state_values_{};
  std::vector<double> ik_solution_{};

  bool is_halted = false;
};
}  // namespace nova_path_planner

#endif  // NOVA_PATH_PLANNER__NOVA_PATH_PLANNER_HPP_
