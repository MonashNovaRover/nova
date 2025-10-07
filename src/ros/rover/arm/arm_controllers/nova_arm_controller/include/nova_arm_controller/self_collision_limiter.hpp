#ifndef NOVA_ARM_CONTROLLER__SELF_COLLISION_LIMITER_HPP_
#define NOVA_ARM_CONTROLLER__SELF_COLLISION_LIMITER_HPP_

#include "joint_limits/joint_limiter_interface.hpp"
#include <moveit/robot_model/robot_model.hpp>
#include <moveit/robot_model_loader/robot_model_loader.hpp>
#include <moveit/planning_scene/planning_scene.hpp>
#include <moveit/robot_state/robot_state.hpp>
#include <moveit/collision_detection/collision_common.hpp>
#include "rclcpp/duration.hpp"
#include <string>

namespace nova_arm_controller
{
class SelfCollisionLimiter : public joint_limits::JointLimiterInterface<trajectory_msgs::msg::JointTrajectoryPoint>
{
public:
  SelfCollisionLimiter() = default;
  ~SelfCollisionLimiter();
  bool on_init() override { return true; }
  bool on_configure(const trajectory_msgs::msg::JointTrajectoryPoint &) override;

  // we replace init() as we don't need the rosparams limits, and we want to get the urdf.
  // be aware that the string argument is the urdf contents, not the topic name
  bool init(
    const std::vector<std::string> & joint_names,
    const rclcpp::node_interfaces::NodeParametersInterface::SharedPtr & param_itf,
    const rclcpp::node_interfaces::NodeLoggingInterface::SharedPtr & logging_itf,
    const std::string & robot_description) override;

  using joint_limits::JointLimiterInterface<trajectory_msgs::msg::JointTrajectoryPoint>::init; // ensure wrappers are in scope

  bool on_enforce(
      const trajectory_msgs::msg::JointTrajectoryPoint & current_joint_states,
      trajectory_msgs::msg::JointTrajectoryPoint & desired_joint_states,
      const rclcpp::Duration & dt) override;

  void reset_internals() override { return; } //TODO: is there anything we need to reset?

private:
  std::string construct_srdf_fallback_string(const urdf::ModelInterfaceSharedPtr &urdf_model, std::string joint_group_name);

  std::string urdf_str_;

  moveit::core::RobotModelPtr robot_model_;
  planning_scene::PlanningScene *planning_scene_ = NULL;
  collision_detection::CollisionRequest collision_request_;
  collision_detection::CollisionResult collision_result_;

  /**
   * @brief Automatically generates an allowed collision matrix in the planning_scene_ that ignores self intersections
   * between joints that always self intersect by populating it with intersections from the arm's zero pose.
   */
  void generate_allowed_collision_matrix();

};
} // namespace nova_arm_controller
#endif // NOVA_ARM_CONTROLLER__SELF_COLLISION_LIMITER_HPP_
