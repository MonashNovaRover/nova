/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

A ros2_control controller for Banksia's robotic
  arm payload, which generate paths for the arm to
  follow to navigate between two points virtual
  target end effector poses
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
CONTROLLER: nova_path_planner/NovaPathPlanner
SUBSCRIPTIONS:
  -
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:  nova_path_planner
AUTHOR:   Bailey Chessum
CREATION: 18/05/2025
EDITED:	  18/05/2025
EDITED BY:
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - Remove RPY rotation
 - Apply twist relative to the header.frame_id
   reference frame, with the endeffector as
   default
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

#ifndef NOVA_PATH_PLANNER__NOVA_PATH_PLANNER_HPP_
#define NOVA_PATH_PLANNER__NOVA_PATH_PLANNER_HPP_

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
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/state.hpp"
#include "rclcpp/node.hpp"
#include "realtime_tools/realtime_box.h"
#include "tf2_msgs/msg/tf_message.hpp"
#include "tf2/LinearMath/Scalar.h"
#include <tf2/LinearMath/Transform.h>
#include <tf2_ros/transform_broadcaster.h>
#include "tf2_ros/transform_listener.h"
#include "tf2_ros/buffer.h"
#include <moveit/robot_model/robot_model.h>
#include <moveit/robot_model_loader/robot_model_loader.h>
#include <moveit/robot_state/robot_state.h>
#include <pluginlib/class_loader.hpp>
#include <stdexcept>
#include <moveit/planning_scene/planning_scene.h>
#include <moveit/collision_detection/collision_common.h>
#include <moveit/planning_interface/planning_interface.h>
#include "rclcpp_action/rclcpp_action.hpp"
#include "nova_interfaces/action/arm_plan_path.hpp"

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

protected:
  struct JointHandle
  {
    std::string name;
    std::reference_wrapper<hardware_interface::LoanedStateInterface> state_pos;
    std::reference_wrapper<hardware_interface::LoanedCommandInterface> command;
  };

  /**
   * @brief finds references to state and command interfaces for each joint, constructing  registered_joint_handles_.
   *
   * This will NOT order the joints correctly for MoveIt2.
   */
  controller_interface::CallbackReturn configure_joints();

  /**
   * @brief gets the current value for all position state interfaces from registerd_joint_handles_, in the same order as
   * in registerd_joint_handles_.
   *
   * @returns A new vector of doubles containing the current position state interface value of each joint.
   */
  std::vector<double> get_state_pos_values();

  /**
   * @brief Integrates to update path_planner_pose_ based on the current received_twist_stamped_ptr_ value
   *
   * This will NOT order the joints correctly for MoveIt2.
   */
  void update_path_planner_pose(const rclcpp::Time &time, const rclcpp::Duration &period);

  /**
   * @brief Creates an rclcpp::Node to give to the kinematics_sovler_ plugin, as we can't give it an
   * rclcpp_lifecycle::LifecycleNode::SharedPtr (superclass of the controller).
   */
  rclcpp::Node::SharedPtr create_compat_node_from_lifecycle(const rclcpp_lifecycle::LifecycleNode::SharedPtr& lifecycle_node);

  /**
   * @brief formats command interface names correctly for a given joint, varying with params_.chained_controller_name
   * @returns A string in the format of "chained_controller_name/joint_name/position", or "chained_controller_name" if
   * no chained_controller_name is given
   */
  std::string joint_to_command_interface_name(const std::string& joint_name) const;

  /**
   * @brief Clears pointers to most stateful things created in on_configure
   */
  bool reset();

  /**
   * @brief Resets the TwistStamped, so that update_path_planner_pose stops moving the path_planner_pose_.
   */
  void halt();

  /**
   * @brief Publishes path_planner_pose_ to tf2
   *
   * @param[in]  the time to stamp the published Transform with.
   */
  void publish_to_tf2(const rclcpp::Time &time);

  /**
   * @brief Generates an SRDF string for use with MoveIt2 libraries, based on params_.joint_names
   *
   * Note: order of joints in the joint group specified in the SRDF / in params_.joint_names will NOT necessarily match
   * the order of joint in the final joint group used by MoveIt2. Since J3 is not part of the kinematic chain in
   * Banksia's arm, it will be moved to the end of the joint group, for example. Joint handles must be rearranged in
   * this final order.
   *
   * @param[in]  urdf_model         URDF model, used to find the name of the robot in the SRDF
   * @param[out] joint_group_name   The name given to the joint group
   * @return A string containing an XML format SRDF
   */
  std::string construct_srdf_fallback_string(const urdf::ModelInterfaceSharedPtr &urdf_model,
                                             std::string joint_group_name);

  /**
   * @brief Checks if moving from one pose to another pose would cause a self intersection.
   *
   * This helps to prevent unsafe maneuvers from being executed where a self-intersecting state is 'jumped over' by the
   * kinematics solver, by computing in-between steps between poses of at most self_intersection_max_step_size radians
   * per joint.
   *
   * @param[in]  seed_state         Current positions for each joint in the joint group.
   * @param[in]  target_positions   Target positions for each joint in the joint group.
   * @return True if the given joint_positions cause a self intersection, false otherwise.
   */
  bool check_path_for_self_intersection(const std::vector<double> &seed_state,
                                        const std::vector<double> &target_positions);

  /**
   * @brief Checks if a given pose is self intersecting.
   *
   * @param[in]  joint_positions    Positions for each joint in the joint group to check for self intersections.
   * @return True if the given joint_positions cause a self intersection, false otherwise.
   */
  bool check_pose_for_self_intersection(const std::vector<double>& joint_positions);

  /**
   * @brief Automatically generates an allowed collision matrix in the planning_scene_ that ignores self intersections
   * between joints that always self intersect by populating it with intersections from the arm's zero pose.
   */
  void generate_allowed_collision_matrix();


  rclcpp_action::GoalResponse handle_action_goal(const rclcpp_action::GoalUUID & uuid, std::shared_ptr<const ArmPlanPath::Goal> goal);

  void handle_action_accepted(const std::shared_ptr<GoalHandleArmPlanPath>& goal_handle);

  rclcpp_action::CancelResponse handle_action_cancelled(const std::shared_ptr<GoalHandleArmPlanPath>& goal_handle);

  void execute_action(const std::shared_ptr<GoalHandleArmPlanPath> goal_handle);

  static Vector3d lerp(Vector3d a, Vector3d b, double t);
  static Vector3d lerp2(Vector3d a, Vector3d b, Vector3d c, double t);
  static Vector3d lerp3(Vector3d a, Vector3d b, Vector3d c, Vector3d d, double t);

  static Eigen::Quaterniond slerp(Eigen::Quaterniond a, Eigen::Quaterniond b, double t);
  static Eigen::Quaterniond slerp2(Eigen::Quaterniond a, Eigen::Quaterniond b, Eigen::Quaterniond c, double t);
  static Eigen::Quaterniond slerp3(Eigen::Quaterniond a, Eigen::Quaterniond b, Eigen::Quaterniond c, Eigen::Quaterniond d, double t);

  static Eigen::Isometry3d lerp3(Eigen::Isometry3d a, Eigen::Isometry3d b, Eigen::Isometry3d c, Eigen::Isometry3d d, double t);

  /// Holds command and state interfaces for each joint
  std::vector<JointHandle> registered_joint_handles_;

  // Parameters from ROS for nova_diff_drive_controller
  std::shared_ptr<ParamListener> param_listener_;
  Params params_;

  // Path input
  rclcpp_action::Server<nova_interfaces::action::ArmPlanPath>::SharedPtr action_server_;
  realtime_tools::RealtimeBox<std::shared_ptr<std::queue<std::vector<tf2::Transform>>>> path_{nullptr};
  bool is_path_being_executed_ = false;

  /// Result of the path_planner, and input to IK. Desired position and orientation of the end effector relative to the base.
  tf2::Transform path_planner_pose_ = tf2::Transform();
  rclcpp::Time path_planner_pose_update_time_ = rclcpp::Time();

  // broadcasting path_planner
  std::shared_ptr<tf2_ros::TransformBroadcaster> path_planner_pose_tf_broadcaster_;

  // MoveIt2 Structures
  /// The URDF model for the arm. Needs to exist for the lifecycle of the kinematics_solver_ and robot_model_.
  std::shared_ptr<urdf::ModelInterface> urdf_model_;
  /// The SRDF model for the arm. Needs to exist for the lifecycle of the kinematics_solver_ and robot_model_.
  std::shared_ptr<srdf::Model> srdf_model_;
  /// Model of the arm used for kinematics.
  moveit::core::RobotModelPtr robot_model_;
  /// Name of the joint group used for kinematics, containing all params_.joint_names (but not necessarily in the same
  /// order!!!)
  std::string joint_group_name_;

  // Self intersection check structures
  /// Structure that allows for intersection checks
  planning_scene::PlanningScenePtr planning_scene_;

  // Kinematics Plugin Structures
  /// Compatability node allowing for dependency injection to the MoveIt2 kinematics plugin, as we can't use a
  /// LifecycleNode for this purpose.
  rclcpp::Node::SharedPtr kinematics_compat_node_;
  /// Loads the kinematics_solver_, and needs to stay alive during the whole lifecycle of the kinematics_solver_.
  std::unique_ptr<pluginlib::ClassLoader<kinematics::KinematicsBase>> kinematics_solver_loader_;
  /// MoveIt2 plugin for solving forward and inverse kinematics.
  std::shared_ptr<kinematics::KinematicsBase> kinematics_solver_;

  /*
  std::unique_ptr<pluginlib::ClassLoader<planning_interface::PlannerManager>> planner_loader_;
  std::shared_ptr<planning_interface::PlannerManager> planner_;
  */

  // Timeout to consider cmd_vel commands old
  bool subscriber_is_active_ = false; // not sure what this is for yet
  rclcpp::Time previous_update_timestamp_{0};

  // publish rate limiter
  bool is_halted = false;
};
} // namespace nova_path_planner
#endif // NOVA_PATH_PLANNER__NOVA_PATH_PLANNER_HPP_
