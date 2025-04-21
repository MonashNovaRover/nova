#ifndef NOVA_ARM_CONTROLLER__SELF_COLLISION_LIMITER_HPP_
#define NOVA_ARM_CONTROLLER__SELF_COLLISION_LIMITER_HPP_

#include "joint_limits/joint_limiter_interface.hpp"
#include <moveit/robot_model/robot_model.h>
#include <moveit/robot_model_loader/robot_model_loader.h>
#include <moveit/planning_scene/planning_scene.h>
#include <moveit/robot_state/robot_state.h>
#include <moveit/collision_detection/collision_common.h>
#include "rclcpp/duration.hpp"
#include <string>

namespace nova_arm_controller
{
class SelfCollisionLimiter : public joint_limits::JointLimiterInterface<joint_limits::JointLimits>
{
public:
  SelfCollisionLimiter() = default;
  ~SelfCollisionLimiter();
  bool on_init() override { return true; }
  bool on_configure(const joint_limits::JointLimitsStateDataType &) override;

  // we replace init() as we don't need the rosparams limits, and we want to get the urdf.
  // be aware that the string argument is the urdf contents, not the topic name
  bool init(
    const std::vector<std::string> & joint_names,
    const rclcpp::node_interfaces::NodeParametersInterface::SharedPtr & param_itf,
    const rclcpp::node_interfaces::NodeLoggingInterface::SharedPtr & logging_itf,
    const std::string & robot_description) override;

  using joint_limits::JointLimiterInterface<joint_limits::JointLimits>::init; // ensure wrappers are in scope

  bool on_enforce(
      joint_limits::JointLimitsStateDataType & current_joint_states,
      joint_limits::JointLimitsStateDataType & desired_joint_states,
      const rclcpp::Duration & dt) override;

  // We don't limit effort.
  bool on_enforce(std::vector<double> &) override { return false; }

private:
  std::string construct_srdf_fallback_string(const urdf::ModelInterfaceSharedPtr &urdf_model, std::string joint_group_name);

  std::string urdf_str;

  moveit::core::RobotModelPtr robot_model;
  planning_scene::PlanningScene *planning_scene;
  collision_detection::CollisionRequest collision_request;
  collision_detection::CollisionResult collision_result;

};
} // namespace nova_arm_controller
#endif // NOVA_ARM_CONTROLLER__SELF_COLLISION_LIMITER_HPP_
